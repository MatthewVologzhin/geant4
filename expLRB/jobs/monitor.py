#!/usr/bin/env python3
"""expLRB HTCondor monitor (per-job lifecycle).

Shows every expLRB array task with ID, status, progress bar, events done/total,
elapsed, ETA and speed, plus a cluster-wide total. Live progress is read from the
running job's remote stdout via `condor_tail` (compute nodes mount $HOME read-only,
so the captured .out reaches the submit dir only when the job ends); a local
logs/expLRB_<scenario>_<proc>.out is used as a fallback once it exists.

Usage:
  jobs/monitor.py                 # interactive (curses) dashboard
  jobs/monitor.py --static        # print one snapshot and exit
  jobs/monitor.py --refresh 5     # refresh interval (seconds, default 3)
  jobs/monitor.py --scenario test_dedx
"""
import argparse
import json
import os
import re
import subprocess
import sys
import time
from datetime import timedelta

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BATCH_PREFIX = "expLRB_"
EVENT_RE = re.compile(r"(?:G4WT\d+\s*>\s*)?(?:-->\s*)?Event\s+(\d+)\s+starts", re.IGNORECASE)
COMPLETED_CACHE = {}
STATUS = {1: "IDLE", 2: "RUN", 3: "REMOVED", 4: "DONE", 5: "HELD", 6: "SUSP"}


def parse_args():
    p = argparse.ArgumentParser(description="expLRB HTCondor monitor")
    p.add_argument("-s", "--static", action="store_true", help="print a single snapshot and exit")
    p.add_argument("-a", "--all", action="store_true", help="also include jobs not launched from this repo")
    p.add_argument("-r", "--refresh", type=int, default=3, help="refresh interval, seconds (default 3)")
    p.add_argument("--scenario", default=None, help="only this scenario")
    return p.parse_args()


def scenario_of(job_ad):
    b = str(job_ad.get("JobBatchName", ""))
    return b[len(BATCH_PREFIX):] if b.startswith(BATCH_PREFIX) else None


def total_events_of(job_ad):
    args = str(job_ad.get("Args") or job_ad.get("Arguments") or "")
    toks = args.split()
    # job_run_one.sh <macro> <seed> <events> <out> <threads>
    if len(toks) >= 3:
        try:
            return int(toks[2])
        except ValueError:
            pass
    return 0


def condor_tail_progress(job_id):
    try:
        res = subprocess.run(["condor_tail", "-maxbytes", "200000", job_id],
                             capture_output=True, text=True, timeout=8)
    except Exception:
        return 0
    return max_event_in_text(res.stdout)


def local_log_progress(scenario, proc):
    path = os.path.join(ROOT, "logs", "expLRB_%s_%s.out" % (scenario, proc))
    if not os.path.exists(path):
        return 0
    try:
        with open(path, "rb") as f:
            f.seek(0, os.SEEK_END)
            size = f.tell()
            f.seek(max(0, size - 65536))
            text = f.read().decode("utf-8", errors="ignore")
        return max_event_in_text(text)
    except Exception:
        return 0


def max_event_in_text(text):
    best = 0
    for line in text.splitlines():
        m = EVENT_RE.search(line)
        if m:
            v = int(m.group(1))
            if v > best:
                best = v
    return best


def get_jobs(show_all, scenario_filter):
    try:
        res = subprocess.run(
            ["condor_q", "-json", "-attributes",
             "ClusterId,ProcId,JobStatus,JobStartDate,QDate,Iwd,Out,Args,JobBatchName"],
            capture_output=True, text=True, timeout=8)
        raw = json.loads(res.stdout) if res.stdout.strip() else []
    except Exception:
        raw = []

    jobs = {}
    active = set()
    for ad in raw:
        scen = scenario_of(ad)
        if scen is None:
            if not show_all:
                continue
            iwd = os.path.abspath(str(ad.get("Iwd", "")))
            if iwd != ROOT:
                continue
            scen = "?"
        if scenario_filter and scen != scenario_filter:
            continue
        cid, proc = ad.get("ClusterId"), ad.get("ProcId")
        jid = "%s.%s" % (cid, proc)
        active.add(jid)
        jobs[jid] = {
            "id": jid, "cluster": cid, "proc": proc, "scenario": scen,
            "status": int(ad.get("JobStatus", 1)),
            "start": ad.get("JobStartDate"), "qdate": ad.get("QDate"),
            "total": total_events_of(ad), "done": False,
        }
    # keep finished jobs visible
    for jid, j in list(COMPLETED_CACHE.items()):
        if jid not in active and (not scenario_filter or j["scenario"] == scenario_filter):
            jobs[jid] = j
    for jid, j in jobs.items():
        if not j["done"]:
            COMPLETED_CACHE[jid] = j
    return sorted(jobs.values(), key=lambda x: (str(x["cluster"]), int(x["proc"])))


def metrics(job, now):
    total = job["total"]
    st = job["status"]
    if job["done"] or st == 4:
        job["done"] = True
        return dict(status="DONE", cur=total, total=total, elapsed="Done",
                    eta="00:00:00", speed=0.0)
    if st == 1:
        wait = max(0, int(now - job["qdate"])) if job.get("qdate") else 0
        return dict(status="IDLE", cur=0, total=total,
                    elapsed="Q:" + str(timedelta(seconds=wait)), eta="Queued", speed=0.0)
    if st == 5:
        return dict(status="HELD", cur=0, total=total, elapsed="Held", eta="Error", speed=0.0)
    # RUN (or exiting): 2,3,6
    cur = condor_tail_progress(job["id"])
    if cur == 0:
        cur = local_log_progress(job["scenario"], job["proc"])
    if total > 0:
        cur = min(cur, total)
    elapsed = max(1, int(now - job["start"])) if job.get("start") else 0
    speed = (cur / elapsed) if (elapsed > 0 and cur > 0) else 0.0
    rem = max(0, total - cur)
    if speed > 0 and rem > 0:
        eta = str(timedelta(seconds=int(rem / speed)))
    elif total > 0 and cur >= total:
        eta = "Saving..."
    else:
        eta = "--:--"
    if total > 0 and cur >= total:
        job["done"] = True
    return dict(status="RUN", cur=cur, total=total,
                elapsed=(str(timedelta(seconds=elapsed)) if elapsed else "--:--"),
                eta=eta, speed=speed)


def bar(cur, total, length=16):
    if total <= 0:
        return "[" + "." * length + "]  --.-%"
    pct = min(1.0, cur / total)
    filled = int(length * pct)
    return "[" + "#" * filled + "." * (length - filled) + "] %5.1f%%" % (pct * 100)


def row(job, m):
    ev = "%d/%d" % (m["cur"], m["total"]) if m["total"] > 0 else "%d/???" % m["cur"]
    return " %-12s | %-4s | %-24s | %-14s | %-9s | %-9s | %6.1f ev/s" % (
        job["id"], m["status"], bar(m["cur"], m["total"]), ev, m["elapsed"], m["eta"], m["speed"])


def render(rows, counts, tot_cur, tot_tot, tot_speed):
    out = []
    out.append("expLRB monitor | jobs=%d [ RUN:%d IDLE:%d DONE:%d HELD:%d ]" % (
        counts["n"], counts["RUN"], counts["IDLE"], counts["DONE"], counts["HELD"]))
    batch_eta = "Calculating..."
    rem = sum(max(0, r["total"] - r["cur"]) for r in rows)
    if tot_speed > 0 and rem > 0:
        batch_eta = str(timedelta(seconds=int(rem / tot_speed)))
    elif counts["RUN"] == 0 and counts["IDLE"] == 0 and counts["DONE"] > 0:
        batch_eta = "ALL FINISHED"
    out.append("Total progress: %s | Speed: %7.1f ev/s | Batch ETA: %s" % (
        bar(tot_cur, tot_tot, 24), tot_speed, batch_eta))
    out.append("-" * 100)
    out.append(" %-12s | %-4s | %-24s | %-14s | %-9s | %-9s | %-10s" % (
        "JOB ID", "STAT", "PROGRESS", "EVENTS", "ELAPSED", "ETA", "SPEED"))
    out.append("=" * 100)
    return out


def snapshot(show_all, scenario_filter, now):
    jobs = get_jobs(show_all, scenario_filter)
    rows, counts = [], {"n": len(jobs), "RUN": 0, "IDLE": 0, "DONE": 0, "HELD": 0}
    tot_cur = tot_tot = 0
    tot_speed = 0.0
    for j in jobs:
        m = metrics(j, now)
        rows.append(dict(job=j, **m))
        counts[m["status"]] = counts.get(m["status"], 0) + 1
        tot_cur += m["cur"]
        tot_tot += m["total"]
        tot_speed += m["speed"]
    return rows, counts, tot_cur, tot_tot, tot_speed


def run_static(show_all, scenario_filter):
    rows, counts, tc, tt, ts = snapshot(show_all, scenario_filter, time.time())
    for line in render(rows, counts, tc, tt, ts):
        print(line)
    for r in rows:
        print(row(r["job"], r))


def run_curses(args):
    import curses
    def loop(stdscr):
        curses.curs_set(0)
        stdscr.timeout(args.refresh * 1000)
        scroll = 0
        while True:
            now = time.time()
            rows, counts, tc, tt, ts = snapshot(args.all, args.scenario, now)
            max_y, max_x = stdscr.getmaxyx()
            stdscr.erase()
            header = render(rows, counts, tc, tt, ts)
            for i, line in enumerate(header):
                stdscr.addnstr(i, 0, line, max_x)
            visible = max(1, max_y - len(header) - 1)
            max_scroll = max(0, len(rows) - visible)
            scroll = min(scroll, max_scroll)
            y = len(header)
            for r in rows[scroll:scroll + visible]:
                stdscr.addnstr(y, 0, row(r["job"], r), max_x)
                y += 1
            footer = " [q]uit  [up/down] scroll  [r]efresh  (%d-%d/%d)" % (
                scroll + 1, min(len(rows), scroll + visible), len(rows))
            stdscr.addnstr(max_y - 1, 0, footer.ljust(max_x - 1), max_x, curses.A_REVERSE)
            stdscr.refresh()
            key = stdscr.getch()
            if key in (ord("q"), ord("Q")):
                break
            if key in (curses.KEY_DOWN, ord("j")):
                scroll = min(max_scroll, scroll + 1)
            elif key in (curses.KEY_UP, ord("k")):
                scroll = max(0, scroll - 1)
    curses.wrapper(loop)


def main():
    args = parse_args()
    if args.static or not sys.stdout.isatty():
        run_static(args.all, args.scenario)
    else:
        run_curses(args)


if __name__ == "__main__":
    main()
