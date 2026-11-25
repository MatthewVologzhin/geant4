#!/usr/bin/env bash
# Probe both schedulers and report free CPU capacity.
# Prints machine-readable lines used by jobs/submit.sh:
#   slurm_free=<idle CPUs>
#   condor_free=<available CPUs>
#   recommend=<slurm|condor|none>
set -uo pipefail

slurm_free=0
condor_free=0

# --- SLURM: sum of idle CPUs across partitions (sinfo %C = allocated/idle/other/total) ---
if command -v sinfo >/dev/null 2>&1; then
  slurm_free="$(sinfo -h -o '%C' 2>/dev/null | awk -F/ '{idle += $2} END {printf "%d", idle+0}')"
  slurm_free="${slurm_free:-0}"
fi

# --- HTCondor: sum of CPUs on available (unclaimed/owner) slots ---
if command -v condor_status >/dev/null 2>&1; then
  condor_free="$(condor_status -available -af Cpus 2>/dev/null | awk '{s+=$1} END {printf "%d", s+0}')"
  condor_free="${condor_free:-0}"
fi

echo "slurm_free=${slurm_free}"
echo "condor_free=${condor_free}"

rec="none"
if [[ "$slurm_free" -gt 0 || "$condor_free" -gt 0 ]]; then
  if [[ "$slurm_free" -ge "$condor_free" ]]; then rec="slurm"; else rec="condor"; fi
fi
echo "recommend=${rec}"
