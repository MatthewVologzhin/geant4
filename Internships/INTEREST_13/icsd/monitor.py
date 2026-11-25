#!/usr/bin/env python3
import os
import re
import sys
import time
import json
import curses
import subprocess
import argparse
from datetime import timedelta

# Persistent cache for jobs that finished and disappeared from condor_q
COMPLETED_JOBS_CACHE = {}

def parse_cli_args():
    parser = argparse.ArgumentParser(description="Full Lifecycle Geant4 HTCondor Monitor")
    parser.add_argument("-s", "--static", action="store_true", help="Print all jobs once to stdout without interactive UI")
    parser.add_argument("-a", "--all", action="store_true", help="Show jobs from ALL directories (default: current directory only)")
    parser.add_argument("-r", "--refresh", type=int, default=3, help="Refresh interval in seconds (default: 3s)")
    return parser.parse_args()

def get_all_jobs(show_all_dirs=False):
    """
    Fetches ALL jobs (Idle, Running, Held) and merges with COMPLETED jobs cache.
    """
    global COMPLETED_JOBS_CACHE
    current_dir = os.path.abspath(os.getcwd())
    
    # Query all active jobs (no JobStatus constraint)
    cmd = [
        "condor_q",
        "-json",
        "-attributes", "ClusterId,ProcId,JobStatus,JobStartDate,QDate,EnteredCurrentStatus,BeamOn,Out,Iwd,Arguments,Args"
    ]
    
    try:
        res = subprocess.run(cmd, capture_output=True, text=True, timeout=5)
        raw_jobs = json.loads(res.stdout) if res.stdout.strip() else []
    except Exception:
        raw_jobs = []

    active_job_ids = set()
    jobs_dict = {}

    for job_ad in raw_jobs:
        iwd = os.path.abspath(job_ad.get("Iwd", ""))
        if not show_all_dirs and iwd != current_dir:
            continue

        cluster_id = job_ad.get("ClusterId")
        proc_id = job_ad.get("ProcId")
        job_id = f"{cluster_id}.{proc_id}"
        active_job_ids.add(job_id)
        
        status_code = job_ad.get("JobStatus", 1)
        start_time = job_ad.get("JobStartDate")
        q_date = job_ad.get("QDate")

        # Extract BeamOn
        total_events = 0
        beam_on_val = job_ad.get("BeamOn")
        if beam_on_val is not None and str(beam_on_val) != "undefined":
            try: total_events = int(str(beam_on_val).strip('"'))
            except ValueError: total_events = 0

        # Fallback to arguments
        if total_events == 0:
            args_str = job_ad.get("Arguments") or job_ad.get("Args") or ""
            args_list = args_str.split()
            if len(args_list) > 7:
                try: total_events = int(args_list[7])
                except ValueError: pass

        # Resolve output file
        out_raw = job_ad.get("Out", "")
        resolved_out = (out_raw if os.path.isabs(out_raw) else os.path.join(iwd, out_raw)) if out_raw else None

        job_info = {
            "id": job_id,
            "cluster": cluster_id,
            "proc": proc_id,
            "status_code": status_code,
            "start_time": start_time,
            "q_date": q_date,
            "total_events": total_events,
            "out_file": resolved_out,
            "is_completed": False
        }
        jobs_dict[job_id] = job_info

    # Detect jobs that finished (were in cache/running, but now disappeared from condor_q)
    for job_id, cached_job in list(COMPLETED_JOBS_CACHE.items()):
        if job_id not in active_job_ids:
            # Job is completely done
            jobs_dict[job_id] = cached_job

    # Update cache with currently active jobs for future tracking
    for job_id, job_info in jobs_dict.items():
        if not job_info.get("is_completed"):
            COMPLETED_JOBS_CACHE[job_id] = job_info

    # Sort jobs by ClusterId and ProcId
    all_jobs = list(jobs_dict.values())
    all_jobs.sort(key=lambda x: (x["cluster"], x["proc"]))
    return all_jobs

def read_last_event_from_local_file(filepath):
    if not filepath or not os.path.exists(filepath):
        return 0
    try:
        with open(filepath, "rb") as f:
            f.seek(0, os.SEEK_END)
            filesize = f.tell()
            read_size = min(8192, filesize)
            f.seek(filesize - read_size)
            raw_data = f.read().decode("utf-8", errors="ignore")
            
        pattern = re.compile(r"(?:G4WT\d+\s*>\s*)?(?:-->\s*)?Event\s+(\d+)\s+starts", re.IGNORECASE)
        max_event = 0
        for line in raw_data.splitlines():
            match = pattern.search(line)
            if match:
                ev = int(match.group(1))
                if ev > max_event:
                    max_event = ev
        return max_event
    except Exception:
        return 0

def format_bar(current, total, length=16):
    if total <= 0:
        return f"[{'·' * length}]  --.-%"
    percent = min(1.0, current / total)
    filled = int(length * percent)
    bar = "█" * filled + "░" * (length - filled)
    return f"[{bar}] {percent*100:5.1f}%"

def calculate_metrics(job, now):
    total = job["total_events"]
    status_code = job.get("status_code", 1)
    
    # 1. COMPLETED (disappeared from queue or marked done)
    if job.get("is_completed") or status_code == 4:
        job["is_completed"] = True
        return {
            "status_str": "DONE",
            "curr_event": total,
            "total_events": total,
            "elapsed_str": "Done",
            "eta_str": "00:00:00",
            "speed_str": "-- ev/s",
            "speed_val": 0.0,
            "rem_events": 0
        }

    # 2. IDLE (waiting in queue)
    if status_code == 1:
        wait_sec = max(0, int(now - job["q_date"])) if job.get("q_date") else 0
        return {
            "status_str": "IDLE",
            "curr_event": 0,
            "total_events": total,
            "elapsed_str": f"Q:{str(timedelta(seconds=wait_sec))}",
            "eta_str": "Queued",
            "speed_str": "-- ev/s",
            "speed_val": 0.0,
            "rem_events": total
        }

    # 3. HELD / ERROR
    if status_code == 5:
        return {
            "status_str": "HELD",
            "curr_event": 0,
            "total_events": total,
            "elapsed_str": "Held",
            "eta_str": "Error",
            "speed_str": "-- ev/s",
            "speed_val": 0.0,
            "rem_events": total
        }

    # 4. RUNNING (JobStatus == 2)
    curr_event = read_last_event_from_local_file(job["out_file"])
    start_time = job["start_time"]
    
    elapsed_sec = max(1, int(now - start_time)) if start_time else 0
    elapsed_str = str(timedelta(seconds=elapsed_sec)) if start_time else "--:--"
    
    ev_per_sec = (curr_event / elapsed_sec) if (elapsed_sec > 0 and curr_event > 0) else 0.0
    speed_str = f"{ev_per_sec:5.1f} ev/s" if ev_per_sec > 0 else "-- ev/s"
    
    rem_events = max(0, total - curr_event)
    
    if total > 0 and ev_per_sec > 0:
        rem_sec = int(rem_events / ev_per_sec)
        eta_str = str(timedelta(seconds=rem_sec))
        if curr_event >= total:
            eta_str = "Saving..."
    else:
        eta_str = "--:--"

    # If the process reached target and output file finished
    if total > 0 and curr_event >= total:
        job["is_completed"] = True

    return {
        "status_str": "RUN",
        "curr_event": curr_event,
        "total_events": total,
        "elapsed_str": elapsed_str,
        "eta_str": eta_str,
        "speed_str": speed_str,
        "speed_val": ev_per_sec,
        "rem_events": rem_events
    }

def run_static_mode(show_all):
    jobs = get_all_jobs(show_all)
    if not jobs:
        print("No jobs found for the current directory.")
        return

    now = time.time()
    header = f"{'JOB ID':<9} | {'STAT':<4} | {'PROGRESS':<24} | {'EVENTS':<21} | {'ELAPSED':<9} | {'ETA':<9} | {'SPEED':<10}"
    print(header)
    print("=" * len(header))

    for job in jobs:
        m = calculate_metrics(job, now)
        bar = format_bar(m["curr_event"], m["total_events"])
        ev_str = f"{m['curr_event']}/{m['total_events']}" if m["total_events"] > 0 else f"{m['curr_event']}/???"
        print(f"{job['id']:<9} | {m['status_str']:<4} | {bar:<24} | {ev_str:<21} | {m['elapsed_str']:<9} | {m['eta_str']:<9} | {m['speed_str']:<10}")

def curses_dashboard(stdscr, args):
    curses.curs_set(0)
    stdscr.timeout(args.refresh * 1000)
    
    has_colors = curses.has_colors()
    if has_colors:
        curses.start_color()
        curses.use_default_colors()
        curses.init_pair(1, curses.COLOR_CYAN, -1)     # Headers / Running
        curses.init_pair(2, curses.COLOR_GREEN, -1)    # Done / Finished
        curses.init_pair(3, curses.COLOR_YELLOW, -1)   # Idle / Queued
        curses.init_pair(4, curses.COLOR_RED, -1)      # Held / Error
        curses.init_pair(5, curses.COLOR_WHITE, curses.COLOR_BLUE) # Footer status bar

    scroll_pos = 0

    while True:
        now = time.time()
        jobs = get_all_jobs(args.all)
        max_y, max_x = stdscr.getmaxyx()
        
        stdscr.erase()
        
        # --- 1. Compute Cluster-Wide Metrics ---
        processed_data = []
        tot_curr = 0
        tot_target = 0
        tot_speed = 0.0
        tot_rem_events = 0
        
        count_run = 0
        count_idle = 0
        count_done = 0
        count_held = 0

        for job in jobs:
            m = calculate_metrics(job, now)
            processed_data.append((job, m))
            
            tot_curr += m["curr_event"]
            tot_target += m["total_events"]
            tot_speed += m["speed_val"]
            tot_rem_events += m["rem_events"]
            
            st = m["status_str"]
            if st == "RUN": count_run += 1
            elif st == "IDLE": count_idle += 1
            elif st == "DONE": count_done += 1
            elif st == "HELD": count_held += 1

        # Global ETA = (Total Remaining Events across entire cluster) / (Current Total Speed)
        if tot_speed > 0 and tot_rem_events > 0:
            global_batch_eta = str(timedelta(seconds=int(tot_rem_events / tot_speed)))
        elif count_idle == 0 and count_run == 0 and count_done > 0:
            global_batch_eta = "ALL FINISHED"
        else:
            global_batch_eta = "Calculating..."

        # --- 2. Render Header ---
        cluster_bar = format_bar(tot_curr, tot_target, length=20)
        
        line1 = f" GEANT4 CLUSTER MONITOR | Total: {len(jobs)} [ RUN:{count_run} | IDLE:{count_idle} | DONE:{count_done} | HELD:{count_held} ]"
        line2 = f" Total Progress: {cluster_bar} | Speed: {tot_speed:7.1f} ev/s | Batch ETA: {global_batch_eta}"
        
        stdscr.addnstr(0, 0, line1, max_x, curses.color_pair(1) | curses.A_BOLD if has_colors else curses.A_BOLD)
        stdscr.addnstr(1, 0, line2, max_x, curses.color_pair(1) if has_colors else curses.A_NORMAL)
        stdscr.addnstr(2, 0, "-" * max(0, max_x - 1), max_x)

        # Table Header
        col_header = f" {'JOB ID':<9} | {'STAT':<4} | {'PROGRESS':<24} | {'EVENTS':<21} | {'ELAPSED':<9} | {'ETA':<9} | {'SPEED':<10}"
        stdscr.addnstr(3, 0, col_header, max_x, curses.A_BOLD)
        stdscr.addnstr(4, 0, "=" * max(0, max_x - 1), max_x)

        # --- 3. Render Job Rows (Scrollable) ---
        visible_rows = max(1, max_y - 7)
        max_scroll = max(0, len(processed_data) - visible_rows)
        scroll_pos = min(scroll_pos, max_scroll)
        scroll_pos = max(0, scroll_pos)

        row_idx = 5
        for i in range(scroll_pos, min(len(processed_data), scroll_pos + visible_rows)):
            job, m = processed_data[i]
            bar = format_bar(m["curr_event"], m["total_events"])
            ev_str = f"{m['curr_event']}/{m['total_events']}" if m["total_events"] > 0 else f"{m['curr_event']}/???"
            row_str = f" {job['id']:<9} | {m['status_str']:<4} | {bar:<24} | {ev_str:<21} | {m['elapsed_str']:<9} | {m['eta_str']:<9} | {m['speed_str']:<10}"
            
            # Color assignment per status
            attr = curses.A_NORMAL
            if has_colors:
                if m["status_str"] == "DONE":
                    attr = curses.color_pair(2) | curses.A_BOLD
                elif m["status_str"] == "IDLE":
                    attr = curses.color_pair(3)
                elif m["status_str"] == "HELD":
                    attr = curses.color_pair(4) | curses.A_BOLD

            stdscr.addnstr(row_idx, 0, row_str, max_x, attr)
            row_idx += 1

        # --- 4. Render Footer ---
        footer_y = max_y - 1
        if footer_y > 5:
            scroll_info = f"[{scroll_pos + 1}-{min(len(processed_data), scroll_pos + visible_rows)}/{len(processed_data)}]" if processed_data else "[0/0]"
            help_text = f" [↑/↓/PgUp/PgDn] Scroll | [R] Refresh | [Q] Quit  {scroll_info:>25}"
            stdscr.addnstr(footer_y, 0, help_text.ljust(max_x - 1), max_x, curses.color_pair(5) if has_colors else curses.A_REVERSE)

        stdscr.noutrefresh()
        curses.doupdate()

        # --- 5. Keyboard Navigation ---
        try:
            key = stdscr.getch()
        except KeyboardInterrupt:
            break

        if key in [ord('q'), ord('Q')]:
            break
        elif key in [curses.KEY_UP, ord('k')]:
            scroll_pos = max(0, scroll_pos - 1)
        elif key in [curses.KEY_DOWN, ord('j')]:
            scroll_pos = min(max_scroll, scroll_pos + 1)
        elif key in [curses.KEY_PPAGE, ord('b')]:  # Page Up
            scroll_pos = max(0, scroll_pos - visible_rows)
        elif key in [curses.KEY_NPAGE, ord('f')]:  # Page Down
            scroll_pos = min(max_scroll, scroll_pos + visible_rows)
        elif key in [curses.KEY_HOME, ord('g')]:
            scroll_pos = 0
        elif key in [curses.KEY_END, ord('G')]:
            scroll_pos = max_scroll
        elif key in [ord('r'), ord('R')]:
            continue

def main():
    args = parse_cli_args()
    if args.static:
        run_static_mode(args.all)
    else:
        try:
            curses.wrapper(curses_dashboard, args)
        except KeyboardInterrupt:
            pass

if __name__ == "__main__":
    main()
