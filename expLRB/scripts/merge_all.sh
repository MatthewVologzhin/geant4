#!/usr/bin/env bash
# Merge every event-chunk file of one run tag into a single ROOT file.
# Usage: scripts/merge_all.sh <run_tag> [merged_dir] [raw_dir]
# merged_dir defaults to output/root, where plot.C expects <scenario>.root.
set -euo pipefail
SELF_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

TAG="${1:?usage: merge_all.sh <run_tag> [merged_dir] [raw_dir]}"
MERGED_DIR="${2:-output/root}"
RAW_DIR="${3:-output/raw}"

shopt -s nullglob
all=("$RAW_DIR/${TAG}"_*.root)
shopt -u nullglob

# Skip partially-written / near-empty chunks: while a job is still running its
# ROOT file is a ~290-byte stub (written directly in the shared output dir by the
# legacy runner). Only merge complete chunks (>10 KB).
files=()
for f in "${all[@]}"; do
  sz=$(stat -c%s "$f" 2>/dev/null || echo 0)
  if [[ "$sz" -gt 10000 ]]; then files+=("$f"); else echo "[merge_all] skip in-progress/stub: $f ($sz B)" >&2; fi
done

if [[ "${#files[@]}" -eq 0 ]]; then
  echo "[merge_all] no complete chunk files matching $RAW_DIR/${TAG}_*.root" >&2
  exit 1
fi

mkdir -p "$MERGED_DIR"
echo "[merge_all] merging ${#files[@]} files for tag '$TAG'"
"$SELF_DIR/merge_runs.sh" "$MERGED_DIR/${TAG}.root" "${files[@]}"
