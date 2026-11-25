#!/usr/bin/env bash
# Merge every event-chunk file of one run tag into a single ROOT file.
# Usage: scripts/merge_all.sh <run_tag> [merged_dir] [raw_dir]
set -euo pipefail
SELF_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

TAG="${1:?usage: merge_all.sh <run_tag> [merged_dir] [raw_dir]}"
MERGED_DIR="${2:-output/merged}"
RAW_DIR="${3:-output/raw}"

shopt -s nullglob
files=("$RAW_DIR/${TAG}"_*.root)
shopt -u nullglob

if [[ "${#files[@]}" -eq 0 ]]; then
  echo "[merge_all] no chunk files matching $RAW_DIR/${TAG}_*.root" >&2
  exit 1
fi

mkdir -p "$MERGED_DIR"
echo "[merge_all] merging ${#files[@]} files for tag '$TAG'"
"$SELF_DIR/merge_runs.sh" "$MERGED_DIR/${TAG}.root" "${files[@]}"
