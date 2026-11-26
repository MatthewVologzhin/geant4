#!/usr/bin/env bash
# SLURM job-array worker for one expLRB configuration.
# The array size is provided by jobs/submit.sh via:  sbatch --array=1-N ...
#
#SBATCH --job-name=expLRB
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=10
#SBATCH --time=24:00:00
#SBATCH --output=logs/expLRB_%A_%a.out
#SBATCH --error=logs/expLRB_%A_%a.err
#
# Expected environment (set by jobs/submit.sh through --export):
#   BASE_MACRO      path to the generated base macro
#   RUN_TAG         short config tag (used in output file names)
#   EVENTS_PER_JOB  events per array element
#   THREADS         threads per job
#   RAW_DIR         directory for raw chunk ROOT files (default output/raw)
#   JOB_RUNNER      absolute path to jobs/job_run_one.sh (SLURM spools this
#                   script, so it cannot be located relative to BASH_SOURCE)
set -euo pipefail

: "${BASE_MACRO:?BASE_MACRO env is required (submit via jobs/submit.sh)}"
: "${RUN_TAG:?RUN_TAG env is required}"
EVENTS_PER_JOB="${EVENTS_PER_JOB:-50000}"
THREADS="${THREADS:-${SLURM_CPUS_PER_TASK:-10}}"
RAW_DIR="${RAW_DIR:-output/raw}"
RUN_LABEL="${RUN_LABEL:-$(date +%Y%m%d_%H%M%S)}"
IDX="${SLURM_ARRAY_TASK_ID:?this script must run as a SLURM array}"

# Submission-unique, non-overlapping seed (safe for hadd across independent runs)
SEED=$(( IDX + 1000000 * ( ${SLURM_JOB_ID:-0} % 1000 ) ))
OUT="${RAW_DIR}/${RUN_TAG}_${RUN_LABEL}_$(printf '%04d' "$IDX").root"

mkdir -p "$RAW_DIR" logs

exec "${JOB_RUNNER:?JOB_RUNNER env is required (submit via jobs/submit.sh)}" \
     "$BASE_MACRO" "$SEED" "$EVENTS_PER_JOB" "$OUT" "$THREADS"
