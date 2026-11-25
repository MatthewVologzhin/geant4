#!/usr/bin/env bash
# expLRB cluster submitter driven by config/macros/manifest.tsv.
#
# 1. Reads the scenario matrix generated from config/runs.conf (manifest.tsv).
# 2. Optionally probes SLURM / HTCondor and picks the one with most free CPUs.
# 3. Splits each scenario's events into an array of jobs (own seed + output chunk).
# 4. Prints the per-scenario merge command.
#
# Usage:
#   jobs/submit.sh                       # all scenarios from the manifest
#   jobs/submit.sh monolayer_nofilt_cells_had test_dedx
#   jobs/submit.sh --dry-run             # print planned jobs, submit nothing
#
# Env:
#   EVENTS_TOTAL     total events per configuration (default 1000000)
#   EVENTS_PER_JOB   events per chunk/job         (default 50000 -> 20 jobs)
#   THREADS          threads per job; cluster default is 1 (single-thread)  (default 1)
#   SCHEDULER        auto|slurm|condor            (default auto)
#   MANIFEST         path to manifest.tsv         (default config/macros/manifest.tsv)
#   RAW_DIR          raw chunk output dir         (default output/raw)
#   MERGED_DIR       merged output dir (plot.C reads output/root/<scenario>.root;
#                    default output/root so the pipeline flows into plot.C)
set -euo pipefail

SELF_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SELF_DIR/.." && pwd)"

EVENTS_TOTAL="${EVENTS_TOTAL:-1000000}"
EVENTS_PER_JOB="${EVENTS_PER_JOB:-50000}"
SCHEDULER="${SCHEDULER:-auto}"
MANIFEST="${MANIFEST:-$ROOT/config/macros/manifest.tsv}"
RAW_DIR="${RAW_DIR:-output/raw}"
MERGED_DIR="${MERGED_DIR:-output/root}"

usage() {
  awk 'NR>1 && /^#/ { sub(/^# ?/, ""); print; next } NR>1 { exit }' "$0"
}

DRY_RUN=0
selected=()
for arg in "$@"; do
  case "$arg" in
    --dry-run)      DRY_RUN=1 ;;
    -h|--help)      usage; exit 0 ;;
    -*)             echo "[submit] unknown option: $arg" >&2; exit 2 ;;
    *)              selected+=("$arg") ;;
  esac
done

[[ -f "$MANIFEST" ]] || { echo "[submit] manifest not found: $MANIFEST (run scripts/gen_macros.sh)" >&2; exit 1; }

NJOBS=$(( (EVENTS_TOTAL + EVENTS_PER_JOB - 1) / EVENTS_PER_JOB ))

# --- scheduler selection ---
if [[ "$SCHEDULER" == "auto" ]]; then
  if [[ "$DRY_RUN" == "1" ]]; then
    echo "[submit] dry-run: scheduler auto-probe skipped (set SCHEDULER=slurm|condor to preview a specific backend)"
  else
    probe="$("$SELF_DIR/probe_schedulers.sh")"
    echo "$probe" | sed 's/^/[probe] /'
    SCHEDULER="$(printf '%s\n' "$probe" | awk -F= '/^recommend=/{print $2}')"
  fi
fi
if [[ "$DRY_RUN" != "1" && ( -z "${SCHEDULER:-}" || "$SCHEDULER" == "none" ) ]]; then
  echo "[submit] no scheduler with free resources detected. Force with SCHEDULER=slurm|condor." >&2
  exit 1
fi

# --- read scenarios from the manifest ---
declare -a rows=()
while IFS=$'\t' read -r scenario geometry sample physics scoring events threads macro; do
  [[ -z "$scenario" || "$scenario" == "scenario" ]] && continue
  if [[ "${#selected[@]}" -gt 0 ]]; then
    keep=0
    for want in "${selected[@]}"; do
      [[ "$want" == "$scenario" || "$want" == "$(basename "$macro" .mac)" ]] && keep=1
    done
    [[ "$keep" == "1" ]] || continue
  fi
  rows+=("$scenario|$geometry|$sample|$physics|$scoring|$events|$threads|$macro")
done < "$MANIFEST"

if [[ "${#rows[@]}" -eq 0 ]]; then
  echo "[submit] no scenarios selected (manifest: $MANIFEST)" >&2
  exit 1
fi

mkdir -p "$RAW_DIR" logs "$ROOT/output/submit"

echo "[submit] scheduler=${SCHEDULER}  scenarios=${#rows[@]}  jobs/config=$NJOBS  events/job=$EVENTS_PER_JOB  events/config=$EVENTS_TOTAL"

# --- submit ---
for row in "${rows[@]}"; do
  IFS='|' read -r scenario geometry sample physics scoring events threads macro <<< "$row"
  mac="$ROOT/$macro"
  threads="${THREADS:-1}"   # cluster default: single-thread per task (override with THREADS=N)

  [[ -f "$mac" ]] || { echo "[submit] skip missing macro: $mac" >&2; continue; }

  if [[ "$DRY_RUN" == "1" ]]; then
    if [[ "$SCHEDULER" == "condor" ]]; then
      echo "[dry-run] HTCondor queue $NJOBS -> $scenario  (macro=$macro threads=$threads events/job=$EVENTS_PER_JOB)"
    else
      echo "[dry-run] sbatch --array=1-$NJOBS --cpus-per-task=$threads --job-name=expLRB_$scenario --output=logs/expLRB_${scenario}_%A_%a.out --export=ALL,BASE_MACRO=$macro,EVENTS_PER_JOB=$EVENTS_PER_JOB,THREADS=$threads,RUN_TAG=$scenario,RAW_DIR=$RAW_DIR $SELF_DIR/slurm_array.sh"
    fi
    continue
  fi

  if [[ "$SCHEDULER" == "slurm" ]]; then
    echo "[submit] SLURM array 1-$NJOBS  ->  $scenario  (threads=$threads)"
    sbatch --array=1-"$NJOBS" \
      --cpus-per-task="$threads" \
      --job-name="expLRB_$scenario" \
      --output="logs/expLRB_${scenario}_%A_%a.out" \
      --error="logs/expLRB_${scenario}_%A_%a.err" \
      --export=ALL,BASE_MACRO="$mac",EVENTS_PER_JOB="$EVENTS_PER_JOB",THREADS="$threads",RUN_TAG="$scenario",RAW_DIR="$RAW_DIR" \
      "$SELF_DIR/slurm_array.sh"
  else
    sub="$ROOT/output/submit/${scenario}.sub"
    sed -e "s#@@EXEC@@#${SELF_DIR}/job_run_one.sh#g" \
        -e "s#@@ROOT@@#${ROOT}#g" \
        -e "s#@@BASE_MACRO@@#${mac}#g" \
        -e "s#@@TAG@@#${scenario}#g" \
        -e "s#@@EVENTS@@#${EVENTS_PER_JOB}#g" \
        -e "s#@@THREADS@@#${threads}#g" \
        -e "s#@@OUTDIR@@#${RAW_DIR}#g" \
        -e "s#@@NJOBS@@#${NJOBS}#g" \
        "$SELF_DIR/htcondor.sub.in" > "$sub"
    echo "[submit] HTCondor queue $NJOBS  ->  $scenario  ($sub)"
    condor_submit "$sub"
  fi
done

# --- merge instructions ---
echo
echo "[submit] When all jobs finish, merge each configuration:"
for row in "${rows[@]}"; do
  scenario="${row%%|*}"
  echo "  scripts/merge_all.sh $scenario $MERGED_DIR $RAW_DIR"
done
