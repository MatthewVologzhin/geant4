#!/bin/bash
# ==============================================================================
# analysis.sh
# Script for sequential batch execution of ROOT macros (.C) without a GUI.
# ==============================================================================

set -e # Exit immediately if a command exits with a non-zero status

# 1. Import global project configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [ -f "${SCRIPT_DIR}/project_config.sh" ]; then
    source "${SCRIPT_DIR}/project_config.sh"
else
    echo "ERROR: project_config.sh not found!" >&2
    exit 1
fi

# 2. List of ROOT macros located in scripts/root/ to be executed sequentially
MACROS=(
    #"plotGeometryComparison.C"
    "plotGeometryComparisonTime.C"
    #"plotTimeComparison.C"
    #"plotValidation.C"
)

echo "--> Merging geometry benchmark CSV files..."
BENCHMARK_DIR="${PROJECT_ROOT}/output/root"
MERGED_FILE="${PROJECT_ROOT}/output/csv/geometry_benchmark_all.csv"

# Write the CSV header first
echo "GeomType,Method,VoxelSize_A,Time_s" > "$MERGED_FILE"

# Append data from all individual benchmark files (suppressing error if none exist)
if ls ${BENCHMARK_DIR}/geometry_benchmark_*.csv 1>/dev/null 2>&1; then
    cat ${BENCHMARK_DIR}/geometry_benchmark_*.csv >> "$MERGED_FILE"
    echo "--> Successfully merged benchmark data into ${MERGED_FILE}"
else
    echo "WARNING: No individual geometry benchmark files found in ${BENCHMARK_DIR}"
fi

echo "=== RUNNING ROOT DATA ANALYSIS ==="

# 3. Verify ROOT availability in the system PATH
if ! command -v root >/dev/null 2>&1; then
    echo "ERROR: ROOT is not found in your PATH." >&2
    echo "Please source your ROOT environment first (e.g., source /path/to/root/bin/thisroot.sh)." >&2
    exit 1
fi

# Load Geant4 environment in case macros rely on Geant4 shared libraries
if [ -f "$GEANT4_ENV" ]; then
    source "$GEANT4_ENV"
fi

# 4. Sequential execution of macros
for macro in "${MACROS[@]}"; do
    # Absolute path to the macro inside scripts/root/
    MACRO_PATH="${PROJECT_ROOT}/scripts/root/${macro}"

    if [ -f "$MACRO_PATH" ]; then
        echo "--> Running ROOT macro: ${macro}..."
        # ROOT Command Line Options:
        # -b : Run in batch mode (without rendering graphical GUI windows)
        # -l : Do not show the welcome splash screen
        # -q : Exit immediately after executing the macro
        root -l -b -q "${MACRO_PATH}"
        echo "--> Finished executing ${macro}"
    else
        echo "WARNING: Macro file '${macro}' not found at ${MACRO_PATH}. Skipping." >&2
    fi
done

echo "=== ANALYSIS COMPLETED SUCCESSFULLY ==="