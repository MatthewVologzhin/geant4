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
BIN="${EXP_LRB_BIN:-$(cd "$SELF_DIR/.." && pwd)/build/expLRB}"

[[ -f "$BASE_MACRO" ]] || { echo "[job] base macro not found: $BASE_MACRO" >&2; exit 2; }
[[ -x "$BIN" ]]       || { echo "[job] binary not found/executable: $BIN (run scripts/build.sh)" >&2; exit 2; }

mkdir -p "$(dirname "$OUT")"
echo "[job] base=$(basename "$BASE_MACRO") seed=${SEED} events=${EVENTS} threads=${THREADS} out=${OUT}"
"$BIN" -m "$BASE_MACRO" -t "$THREADS" -s "$SEED" -o "$OUT" --events "$EVENTS"
