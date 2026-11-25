#!/usr/bin/env bash
# Worker for ONE chunk of a run: runs expLRB with a fixed seed and event count.
#
# Usage: job_run_one.sh <base_macro> <seed> <events> <out_root> [threads]
#
# The chunk's event count is passed through --events, which overrides the
# /bio/run/setEvents value in the base macro (CLI wins by design). A single
# base macro therefore splits cleanly across many jobs.
set -euo pipefail

BASE_MACRO="${1:?base macro path}"
SEED="${2:?seed}"
EVENTS="${3:?events}"
OUT="${4:?output root path}"
THREADS="${5:-${THREADS:-10}}"

SELF_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SELF_DIR/.." && pwd)"
BIN="${EXP_LRB_BIN:-$PROJECT_ROOT/build/expLRB}"

# Make the Geant4 runtime environment available on compute nodes (LD_LIBRARY_PATH,
# G4 data paths). Uses config/geant4.conf (GEANT4_PREFIX) unless already set.
if [[ -z "${GEANT4_PREFIX:-}" && -f "$PROJECT_ROOT/config/geant4.conf" ]]; then
  set +u; source "$PROJECT_ROOT/config/geant4.conf"; set -u
fi
if [[ -n "${GEANT4_PREFIX:-}" && -f "$GEANT4_PREFIX/bin/geant4.sh" ]]; then
  set +u; source "$GEANT4_PREFIX/bin/geant4.sh"; set -u
fi

[[ -f "$BASE_MACRO" ]] || { echo "[job] base macro not found: $BASE_MACRO" >&2; exit 2; }
[[ -x "$BIN" ]]       || { echo "[job] binary not found/executable: $BIN (run scripts/build.sh)" >&2; exit 2; }

mkdir -p "$(dirname "$OUT")"

# Force the requested thread count regardless of /run/numberOfThreads in the macro
# (Geant4 honours G4FORCENUMBEROFTHREADS over the UI command).
export G4FORCENUMBEROFTHREADS="$THREADS"

# Mirror the Geant4 output into a shared log on the repository (visible from compute
# nodes) so jobs/monitor.sh can read live progress even when HTCondor returns the
# captured stdout only at job completion.
LOG_BASE="$(basename "$OUT" .root)"
SHARED_LOG="$PROJECT_ROOT/logs/expLRB_${LOG_BASE}.out"
mkdir -p "$(dirname "$SHARED_LOG")"

echo "[job] base=$(basename "$BASE_MACRO") seed=${SEED} events=${EVENTS} threads=${THREADS} out=${OUT}"
"$BIN" -m "$BASE_MACRO" -t "$THREADS" -s "$SEED" -o "$OUT" --events "$EVENTS" 2>&1 | tee "$SHARED_LOG"
