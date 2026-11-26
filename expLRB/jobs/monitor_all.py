#!/usr/bin/env python3
"""Combined expLRB monitor: HTCondor + SLURM + chunk progress in one console view.

The two schedulers run on different hosts that share the same lustre $HOME, so
the repository and output/raw are visible from either. This script queries both
and prints one merged table (per scenario: running / pending / held / done).

Configuration (environment, or a sourced ~/.config/expLRB/monitor.conf):
  CONDOR_SSH     ssh prefix to the HTCondor submit host,
                 e.g. CONDOR_SSH="ssh -o BatchMode=yes user@host"
  SLURM_SSH      ssh prefix to the SLURM login host,
                 e.g. SLURM_SSH="ssh -o BatchMode=yes -p 2222 user@host"
  EXP_LRB_REPO   repo path on the cluster (default ~/projects/geant4/expLRB)
  RAW_DIR        chunk dir, relative to the repo (default output/raw)

If a prefix is empty the local CLI is used when available (condor_q / squeue).
Addresses/users are intentionally NOT stored in the repository.

Usage:
  jobs/monitor_all.py                 # one snapshot
  jobs/monitor_all.py --watch 10      # refresh every 10 s (Ctrl-C to stop)
"""
import argparse
import os
import re
import shlex
import subprocess
import sys
import time
from collections import defaultdict

REPO = os.environ.get("EXP_LRB_REPO", "~/projects/geant4/expLRB")
RAW_DIR = os.environ.get("RAW_DIR", "output/raw")
CONDOR_SSH = os.environ.get("CONDOR_SSH", "").strip()
SLURM_SSH = os.environ.get("SLURM_SSH", "").strip()

CONDOR_STATUS = {1: "PEND", 2: "RUN", 3: "done", 4: "done", 5: "HELD", 6: "SUSP"}
SLURM_STATE = {"PD": "PEND", "R": "RUN", "CG": "RUN", "S": "SUSP",
               "CD": "done", "F": "fail", "CA": "done", "TO": "fail"}


def sh(ssh_prefix, remote_cmd):
    """Run remote_cmd via ssh_prefix, or locally when the prefix is empty."""
    if ssh_prefix:
        cmd = shlex.split(ssh_prefix) + [remote_cmd]
    else:
        cmd = ["bash", "-c", remote_cmd]
    try:
        out = subprocess.run(cmd, capture_output=True, text=True, timeout=25)
        return out.stdout
    except Exception:
        return ""


def strip_batch(name):
    return name[len("expLRB_"):] if name.startswith("expLRB_") else name


def condor_by_scenario():
    if not CONDOR_SSH and not _local("condor_q"):
        return None
    out = sh(CONDOR_SSH, "condor_q -af JobBatchName JobStatus 2>/dev/null")
    agg = defaultdict(lambda: defaultdict(int))
    for line in out.splitlines():
        toks = line.split()
        if len(toks) < 2:
            continue
        try:
            status = int(toks[-1])
        except ValueError:
            continue
        agg[strip_batch(toks[0])][CONDOR_STATUS.get(status, "?")] += 1
    return agg


def slurm_by_scenario():
    if not SLURM_SSH and not _local("squeue"):
        return None
    # -r expands array tasks so each element is counted (otherwise arrays collapse).
    out = sh(SLURM_SSH, "squeue -u \"$USER\" -h -r -o '%j|%t' 2>/dev/null")
    agg = defaultdict(lambda: defaultdict(int))
    for line in out.splitlines():
        if "|" not in line:
            continue
        name, st = line.split("|", 1)
        agg[strip_batch(name.strip())][SLURM_STATE.get(st.strip(), st.strip())] += 1
    return agg


def chunks_by_scenario():
    ssh = CONDOR_SSH or SLURM_SSH  # same shared $HOME either way
    out = sh(ssh, f"ls -1 {REPO}/{RAW_DIR}/*.root 2>/dev/null")
    agg = defaultdict(int)
    for line in out.splitlines():
        base = os.path.basename(line.strip())
        if not base.endswith(".root"):
            continue
        # <scenario>_<RUN_LABEL>_<proc>.root : drop the trailing _<label>_<proc>
        scenario = re.sub(r"_\d{8}_\d{6}_\d+\.root$", "", base)
        scenario = re.sub(r"_\d+\.root$", "", scenario)  # legacy <scenario>_<proc>.root
        agg[scenario] += 1
    return agg


def _local(tool):
    from shutil import which
    return which(tool) is not None


def collect():
    cnd = condor_by_scenario()
    slr = slurm_by_scenario()
    chk = chunks_by_scenario()

    rows = []
    for scen in sorted(set(list((cnd or {}).keys()) + list((slr or {}).keys()) + list(chk.keys()))):
        for sched, agg in (("condor", cnd), ("slurm", slr)):
            if agg and scen in agg:
                d = agg[scen]
                rows.append((sched, scen, d.get("RUN", 0), d.get("PEND", 0),
                             d.get("HELD", 0), d.get("fail", 0), chk.get(scen, 0)))
        if (not cnd or scen not in cnd) and (not slr or scen not in slr):
            rows.append(("-", scen, 0, 0, 0, 0, chk.get(scen, 0)))
    return rows, cnd, slr, chk


def render(rows, cnd, slr, chk):
    lines = []
    ts = time.strftime("%Y-%m-%d %H:%M:%S")
    lines.append(f"expLRB combined monitor    {ts}")
    lines.append(f"condor_ssh={'set' if CONDOR_SSH else 'local'}  "
                 f"slurm_ssh={'set' if SLURM_SSH else 'local'}  repo={REPO}")
    lines.append("-" * 78)
    lines.append(f"{'SCHED':<7}{'SCENARIO':<38}{'RUN':>5}{'PEND':>7}{'HELD':>7}{'FAIL':>6}{'DONE':>6}")
    lines.append("=" * 78)
    tot = {"condor": [0, 0, 0], "slurm": [0, 0, 0]}
    for sched, scen, run, pend, held, fail, done in rows:
        lines.append(f"{sched:<7}{scen:<38}{run:>5}{pend:>7}{held:>7}{fail:>6}{done:>6}")
        if sched in tot:
            tot[sched][0] += run
            tot[sched][1] += pend
            tot[sched][2] += held
    lines.append("-" * 78)
    for sched in ("condor", "slurm"):
        r, p, h = tot[sched]
        note = "" if (cnd if sched == "condor" else slr) is not None else "  (unreachable/skipped)"
        lines.append(f"{sched.upper():<7} run={r:<5} pend={p:<6} held={h}{note}")
    lines.append(f"chunks on disk: {sum(chk.values())}  "
                 f"(scenarios: {len(chk)})")
    return lines


def main():
    ap = argparse.ArgumentParser(description="combined expLRB HTCondor+SLURM monitor")
    ap.add_argument("-w", "--watch", type=int, default=0,
                    help="refresh interval in seconds (0 = one snapshot)")
    args = ap.parse_args()
    try:
        while True:
            rows, cnd, slr, chk = collect()
            out = "\n".join(render(rows, cnd, slr, chk))
            if args.watch > 0:
                sys.stdout.write("\033[2J\033[H")  # clear screen
            print(out, flush=True)
            if args.watch <= 0:
                break
            time.sleep(args.watch)
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
