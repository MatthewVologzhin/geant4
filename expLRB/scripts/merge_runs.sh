#!/usr/bin/env bash
# Безопасное объединение результатов пакетных прогонов expLRB через hadd.
# Объединяйте ТОЛЬКО файлы с независимыми сидами ГПСЧ (--seed у каждого задания),
# иначе суммы по гистограммам будут математически некорректны.
set -euo pipefail

if [ "$#" -lt 2 ]; then
    echo "Usage: $0 <merged_output.root> <run1.root> [run2.root ...]" >&2
    exit 1
fi

OUT="$1"; shift

if ! command -v hadd >/dev/null 2>&1; then
    echo "[merge_runs] ERROR: 'hadd' (ROOT) not found in PATH." >&2
    exit 1
fi

for f in "$@"; do
    if [ ! -f "$f" ]; then
        echo "[merge_runs] ERROR: input file not found: $f" >&2
        exit 1
    fi
    if [ ! -s "$f" ]; then
        echo "[merge_runs] ERROR: input file is empty: $f" >&2
        exit 1
    fi
done

mkdir -p "$(dirname "$OUT")"
echo "[merge_runs] Merging $# files -> $OUT"
hadd -f "$OUT" "$@"
echo "[merge_runs] Done."
