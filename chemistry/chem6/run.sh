#!/bin/bash
# ==============================================================================
# run.sh
# Main execution script: venv preparation, parser execution, and C++ compilation.
# ==============================================================================

set -e # Exit immediately if a command exits with a non-zero status

# 1. Load global project configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [ -f "${SCRIPT_DIR}/project_config.sh" ]; then
    source "${SCRIPT_DIR}/project_config.sh"
else
    echo "ERROR: project_config.sh not found!" >&2
    exit 1
fi

echo "=== PROJECT PREPARATION AND EXECUTION ==="

# 2. Create required directory structure
mkdir -p "${PROJECT_ROOT}/output/logs"
mkdir -p "${PROJECT_ROOT}/output/plots"
mkdir -p "${PROJECT_ROOT}/output/data"

# 5. Compile C++ Geant4 application
chmod +x "${PROJECT_ROOT}/scripts/compile_fast.sh"
bash "${PROJECT_ROOT}/scripts/compile_fast.sh"

echo "=== PREPARATION COMPLETED SUCCESSFULLY ==="

# 6. Simulation execution
# If HTCondor is available on the machine, submit the cluster jobs
if command -v condor_submit >/dev/null 2>&1; then
    echo "--> HTCondor detected. Submitting jobs..."
    condor_submit "${PROJECT_ROOT}/jobs/geant4.sub"
    watch -n 2 condor_q
else
    echo "WARNING: HTCondor is not installed on this system."
    echo "To run simulations locally, execute: ./chem6 run_[JOB_ID].mac"
fi