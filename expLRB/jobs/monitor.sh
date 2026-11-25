#!/usr/bin/env bash
# expLRB cluster progress monitor: per-scenario progress and ETA from Geant4 logs.
#
# For each scenario in config/macros/manifest.tsv it estimates how many events are
# done and the remaining wall time by parsing the Geant4 progress lines in the
# scheduler .out logs and sampling them over time (rate extrapolation:
# rate = Δevents/Δt, ETA = (EVENTS_TOTAL - done)/rate). It runs on the
# login/submit node and only reads small log files, so it consumes no compute-node
# CPU. It also understands single-thread and multi-thread (MT) logs.
#
# Usage:
#   jobs/monitor.sh                 # live watch (refresh every INTERVAL seconds)
#   jobs/monitor.sh --once          # print a single snapshot and exit
#   jobs/monitor.sh --tasks         # also print one line per array task
#   jobs/monitor.sh --interval 30   # refresh period for the watch loop
#   jobs/monitor.sh monolayer_nofilt_cells_had test_dedx   # filter scenarios
#
# Env (keep these equal to the values you passed to jobs/submit.sh):
#   EVENTS_TOTAL    total events per scenario    (default 1000000)
#   EVENTS_PER_JOB  events per array task        (default 50000)
#   THREADS         threads per task (info only) (default 1)
#   MANIFEST        manifest.tsv path            (default config/macros/manifest.tsv)
#   RAW_DIR         raw chunk dir                (default output/raw)
#   LOG_DIR         scheduler logs dir           (default logs)
#   STATE_DIR       sampling state dir           (default output/monitor)
#   SCHEDULER       auto|slurm|condor|none       (default auto)
#   FRESH_SECONDS   log freshness window         (default 900)
set -uo pipefail

SELF_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SELF_DIR/.." && pwd)"

EVENTS_TOTAL="${EVENTS_TOTAL:-1000000}"
EVENTS_PER_JOB="${EVENTS_PER_JOB:-50000}"
THREADS="${THREADS:-1}"
MANIFEST="${MANIFEST:-$ROOT/config/macros/manifest.tsv}"
RAW_DIR="${RAW_DIR:-output/raw}"
LOG_DIR="${LOG_DIR:-logs}"
STATE_DIR="${STATE_DIR:-output/monitor}"
SCHEDULER="${SCHEDULER:-auto}"
FRESH_SECONDS="${FRESH_SECONDS:-900}"

INTERVAL=60
ONCE=0
SHOW_TASKS=0
USE_COLOR=1

usage() { awk 'NR>1 && /^#/ { sub(/^# ?/, ""); print; next } NR>1 { exit }' "$0"; }

sel=()
while [[ $# -gt 0 ]]; do
  case "$1" in
    --once)      ONCE=1 ;;
    --tasks)     SHOW_TASKS=1 ;;
    --interval)  INTERVAL="${2:-60}"; shift ;;
    --no-color)  USE_COLOR=0 ;;
    -h|--help)   usage; exit 0 ;;
    -*)          echo "[monitor] unknown option: $1" >&2; exit 2 ;;
    *)           sel+=("$1") ;;
  esac
  shift
done

[[ -f "$MANIFEST" ]] || { echo "[monitor] manifest not found: $MANIFEST (run scripts/gen_macros.sh)" >&2; exit 1; }

for v in EVENTS_TOTAL EVENTS_PER_JOB FRESH_SECONDS INTERVAL; do
  [[ "${!v}" =~ ^[0-9]+$ ]] || { echo "[monitor] $v must be a positive integer (got: ${!v})" >&2; exit 2; }
done
NJOBS=$(( (EVENTS_TOTAL + EVENTS_PER_JOB - 1) / EVENTS_PER_JOB ))
(( NJOBS > 0 )) || { echo "[monitor] EVENTS_PER_JOB too large" >&2; exit 2; }

# --- scheduler detection ---
if [[ "$SCHEDULER" == "auto" ]]; then
  if command -v squeue >/dev/null 2>&1; then SCHEDULER=slurm
  elif command -v condor_q >/dev/null 2>&1; then SCHEDULER=condor
  else SCHEDULER=none; fi
fi

if [[ "$USE_COLOR" == "1" && -t 1 && -z "${NO_COLOR:-}" ]]; then
  C_DONE=$'\e[32m'; C_RUN=$'\e[33m'; C_PEND=$'\e[90m'; C_STALL=$'\e[31m'; C_RST=$'\e[0m'
else
  C_DONE=""; C_RUN=""; C_PEND=""; C_STALL=""; C_RST=""
fi

# Events done in one log: sum over threads of the max "Event N" seen. Handles both
# prefixed (G4WTn >, MT) and unprefixed (sequential) lines; falls back to the
# per-thread final "Number of events processed : N" summaries for very short runs.
progress_of() {
  local f="$1" total primary
  # 1) Exact total once the task finished: the master summary line (no "[thread").
  total="$(awk '
    /Number of events processed :/ && $0 !~ /\[thread/ {
      if (match($0, /Number of events processed : *[0-9]+/)) {
        s = substr($0, RSTART, RLENGTH); sub(/.*: */, "", s); print s; exit
      }
    }' "$f" 2>/dev/null)"
  if [[ -n "$total" && "$total" =~ ^[0-9]+$ && "$total" -gt 0 ]]; then
    printf '%s' "$total"; return
  fi
  # 2) Live progress during the run: sum over threads of the max "Event N" seen.
  primary="$(awk '
    {
      tid = "0";
      if (match($0, /G4WT[0-9]+/)) tid = substr($0, RSTART + 4, RLENGTH - 4);
      if (match($0, /--> Event [0-9]+/)) {
        n = substr($0, RSTART + 10, RLENGTH - 10) + 0;
        if (n > m[tid]) m[tid] = n;
      }
    }
    END { s = 0; for (t in m) s += m[t]; print s + 0 }' "$f" 2>/dev/null)"
  if [[ -n "$primary" && "$primary" =~ ^[0-9]+$ && "$primary" -gt 0 ]]; then
    printf '%s' "$primary"; return
  fi
  # 3) Fallback: per-thread final summaries.
  awk '
    /\[thread [0-9]+\]/ && match($0, /Number of events processed : [0-9]+/) {
      s += substr($0, RSTART + 29, RLENGTH - 29) + 0;
    }
    END { print s + 0 }' "$f" 2>/dev/null
}

fmt_dur() {  # seconds -> "2h 05m" / "3d 04h" / "<1m"
  local s="$1"
  if ! [[ "$s" =~ ^[0-9]+$ ]]; then printf 'n/a'; return; fi
  if (( s < 60 )); then printf '<1m'; return; fi
  local d=$(( s / 86400 )) h=$(( (s % 86400) / 3600 )) m=$(( (s % 3600) / 60 ))
  if (( d > 0 )); then printf '%dd %02dh' "$d" "$h"
  elif (( h > 0 )); then printf '%dh %02dm' "$h" "$m"
  else printf '%dm' "$m"; fi
}

# Sample one log -> prints "done mtime rate". Persists previous sample in STATE_DIR.
sample_task() {
  local f="$1" now mt d k pmt=0 pd=0 pr=0 rate=0 dt=0
  [[ -f "$f" ]] || { printf '0 0 0'; return; }
  now="$(date +%s)"
  mt="$(stat -c %Y "$f" 2>/dev/null || echo "$now")"
  d="$(progress_of "$f")"; [[ "$d" =~ ^[0-9]+$ ]] || d=0
  (( d > EVENTS_PER_JOB )) && d=$EVENTS_PER_JOB
  k="$STATE_DIR/$(basename "$f").state"
  if [[ -f "$k" ]]; then read -r pmt pd pr < "$k" || true; fi
  [[ "$pmt" =~ ^[0-9]+$ ]] || pmt=0
  [[ "$pd" =~ ^[0-9]+$ ]] || pd=0
  if (( mt > pmt && d > pd )); then
    dt=$(( mt - pmt ))
    rate="$(awk -v dd=$(( d - pd )) -v dt="$dt" 'BEGIN { if (dt > 0) printf "%.4f", dd/dt; else print 0 }')"
  fi
  if awk -v r="$rate" 'BEGIN { exit !(r > 0) }'; then
    if awk -v p="$pr" 'BEGIN { exit !(p > 0) }'; then
      rate="$(awk -v a=0.3 -v r="$rate" -v p="$pr" 'BEGIN { printf "%.4f", a*r + (1-a)*p }')"
    fi
  else
    rate="$pr"; [[ "$rate" =~ ^[0-9.]+$ ]] || rate=0
  fi
  mkdir -p "$STATE_DIR"
  printf '%s %s %s\n' "$mt" "$d" "$rate" > "$k" 2>/dev/null || true
  printf '%s %s %s' "$d" "$mt" "$rate"
}

# scheduler per-scenario running/pending counts (best effort)
declare -A SCHED_RUN SCHED_PEND
sched_snapshot() {
  SCHED_RUN=(); SCHED_PEND=()
  if [[ "$SCHEDULER" == "slurm" ]]; then
    local j t
    while read -r j t; do
      [[ -z "$j" ]] && continue
      local sc="${j#expLRB_}"
      case "$t" in
        R|CG|CD) (( SCHED_RUN["$sc"]++ )) ;;
        PD)      (( SCHED_PEND["$sc"]++ )) ;;
      esac
    done < <(squeue -u "$USER" -h -o "%j %t" 2>/dev/null)
  elif [[ "$SCHEDULER" == "condor" ]]; then
    local b s
    while read -r b s; do
      [[ -z "$b" ]] && continue
      local sc="${b#expLRB_}"
      case "$s" in 2) (( SCHED_RUN["$sc"]++ )) ;; 1) (( SCHED_PEND["$sc"]++ )) ;; esac
    done < <(condor_q -nobatch -af JobBatchName JobStatus 2>/dev/null)
  fi
}

snapshot() {
  sched_snapshot
  local now; now="$(date +%s)"

  local hdr
  printf '%-30s %6s %6s %6s %6s  %-19s %10s  %-10s\n' \
    "scenario" "tasks" "done" "run" "pend" "events done/total" "rate ev/s" "ETA"
  printf '%s\n' "----------------------------------------------------------------------------------------------------"

  local tot_tasks=0 tot_done=0 tot_run=0 tot_pend=0 tot_ev=0 tot_rate=0

  local scenario
  while IFS=$'\t' read -r scenario geometry sample physics scoring events threads macro; do
    [[ -z "$scenario" || "$scenario" == "scenario" ]] && continue
    if [[ "${#sel[@]}" -gt 0 ]]; then
      local keep=0 w
      for w in "${sel[@]}"; do [[ "$w" == "$scenario" ]] && keep=1; done
      [[ "$keep" == "1" ]] || continue
    fi

    shopt -s nullglob
    local logs=("$LOG_DIR/expLRB_${scenario}_"*.out)
    local chunks=("$RAW_DIR/${scenario}_"*.root)
    shopt -u nullglob
    local nlogs="${#logs[@]}" nchunks="${#chunks[@]}"

    local sc_ev=0 sc_rate=0 sc_run=0 sc_stall=0
    local log
    for log in "${logs[@]}"; do
      local d mt rate
      read -r d mt rate < <(sample_task "$log")
      sc_ev=$(( sc_ev + d ))
      if (( d >= EVENTS_PER_JOB )); then
        :
      elif (( now - mt <= FRESH_SECONDS )); then
        sc_run=$(( sc_run + 1 ))
        sc_rate="$(awk -v a="$sc_rate" -v r="$rate" 'BEGIN { printf "%.4f", a + r }')"
      else
        sc_stall=$(( sc_stall + 1 ))
      fi
      if [[ "$SHOW_TASKS" == "1" ]]; then
        local t_eta="n/a"
        if awk -v r="$rate" 'BEGIN { exit !(r > 0) }'; then
          t_eta="$(fmt_dur "$(awk -v rem=$(( EVENTS_PER_JOB - d )) -v r="$rate" 'BEGIN { printf "%d", (rem>0?rem:0)/r }')")"
        fi
        local st="run"; (( d >= EVENTS_PER_JOB )) && st="done"; (( now - mt > FRESH_SECONDS && d < EVENTS_PER_JOB )) && st="stall"
        printf '    %-40s done=%-7s %-6s ETA %s\n' "$(basename "$log")" "$d" "$st" "$t_eta"
      fi
    done

    # finished tasks whose log was removed still count as full chunks
    local extra=$(( nchunks - nlogs )); (( extra < 0 )) && extra=0
    sc_ev=$(( sc_ev + extra * EVENTS_PER_JOB ))

    local sc_done=$nchunks
    local sc_run_final=$sc_run
    local srun="${SCHED_RUN[$scenario]:-0}"
    local spend="${SCHED_PEND[$scenario]:-0}"
    (( srun > sc_run_final )) && sc_run_final=$srun
    local sc_pend=$(( NJOBS - sc_done - sc_run_final )); (( sc_pend < 0 )) && sc_pend=0
    (( spend > sc_pend )) && sc_pend=$spend

    local sc_eta="n/a" sc_eta_s=""
    if (( sc_ev >= EVENTS_TOTAL )); then
      sc_eta="done"; sc_eta_s=0
    elif awk -v r="$sc_rate" 'BEGIN { exit !(r > 0) }'; then
      sc_eta_s="$(awk -v rem=$(( EVENTS_TOTAL - sc_ev )) -v r="$sc_rate" 'BEGIN { printf "%d", (rem>0?rem:0)/r }')"
      sc_eta="$(fmt_dur "$sc_eta_s")"
      (( sc_stall > 0 )) && sc_eta="${sc_eta}+?"
    fi

    local cdone="$C_DONE" crun="$C_RUN"
    (( sc_stall > 0 )) && crun="$C_STALL"
    printf '%-30s %6d %s%6d%s %s%6d%s %s%6d%s  %-19s %10s  %-10s\n' \
      "$scenario" "$NJOBS" \
      "$cdone" "$sc_done" "$C_RST" "$crun" "$sc_run_final" "$C_RST" "$C_PEND" "$sc_pend" "$C_RST" \
      "$sc_ev/$EVENTS_TOTAL" "$sc_rate" "$sc_eta"

    tot_tasks=$(( tot_tasks + NJOBS ))
    tot_done=$(( tot_done + sc_done ))
    tot_run=$(( tot_run + sc_run_final ))
    tot_pend=$(( tot_pend + sc_pend ))
    tot_ev=$(( tot_ev + sc_ev ))
    tot_rate="$(awk -v a="$tot_rate" -v r="$sc_rate" 'BEGIN { printf "%.4f", a + r }')"
  done < "$MANIFEST"

  printf '%s\n' "----------------------------------------------------------------------------------------------------"
  local tn=$(( tot_tasks / NJOBS ))
  local tot_total_ev=$(( tn * EVENTS_TOTAL ))
  local tot_eta="n/a"
  if (( tot_total_ev > 0 && tot_ev >= tot_total_ev )); then
    tot_eta="done"
  elif awk -v r="$tot_rate" 'BEGIN { exit !(r > 0) }'; then
    tot_eta="$(fmt_dur "$(awk -v rem=$(( tot_total_ev - tot_ev )) -v r="$tot_rate" 'BEGIN { printf "%d", (rem>0?rem:0)/r }')")"
  fi
  printf '%-30s %6d %6d %6d %6d  %-19s %10s  %-10s\n' \
    "TOTAL" "$tot_tasks" "$tot_done" "$tot_run" "$tot_pend" \
    "$tot_ev/$tot_total_ev" "$tot_rate" "$tot_eta"
  printf '[monitor] scheduler=%s  scenarios=%d  tasks=%d  events/job=%d  threads=%d\n' \
    "$SCHEDULER" "$tn" "$tot_tasks" "$EVENTS_PER_JOB" "$THREADS"
}

if [[ "$ONCE" == "1" ]]; then
  snapshot
else
  while true; do
    printf '\033[H\033[J'   # clear screen
    printf '[monitor] refreshed %s  (Ctrl-C to exit)\n\n' "$(date '+%Y-%m-%d %H:%M:%S')"
    snapshot
    sleep "$INTERVAL"
  done
fi
