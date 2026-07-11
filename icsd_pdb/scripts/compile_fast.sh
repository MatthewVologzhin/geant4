#!/bin/bash
# ==============================================================================
# compile_fast.sh
# Fast compilation script for CMake/Geant4 projects using local /tmp space.
# Uses project_config.sh for global project variables.
# ==============================================================================

set -e # Exit immediately on error

# Source the unified global configuration (located one level up)
source "$(dirname "${BASH_SOURCE[0]}")/../project_config.sh"

NUM_CORES=$(nproc 2>/dev/null || echo 4)

echo "=== C++ GEANT4 COMPILATION IN /tmp ==="
echo "Project Name:    ${PROJECT_NAME}"
echo "Source Dir:      ${PROJECT_ROOT}"
echo "Temp Build:      ${TEMP_BUILD_DIR}"

# Check for code/config updates since last compilation
NEEDS_COMPILE=false
TARGET_BINARY="${LOCAL_BUILD_DIR}/${BINARY_NAME}"

if [ ! -f "${TARGET_BINARY}" ]; then
    NEEDS_COMPILE=true
else
    MONITOR_PATHS=""
    for path in "src" "include" "CMakeLists.txt"; do
        if [ -e "${PROJECT_ROOT}/${path}" ]; then
            MONITOR_PATHS="${MONITOR_PATHS} ${PROJECT_ROOT}/${path}"
        fi
    done
    if [ -n "$(find ${MONITOR_PATHS} -type f -newer "${TARGET_BINARY}" 2>/dev/null)" ]; then
        NEEDS_COMPILE=true
    fi
fi

if [ "$NEEDS_COMPILE" = false ]; then
    echo "--> No changes detected. C++ binary is up-to-date."
    exit 0
fi

# Load Geant4 environment
if [ -f "$GEANT4_ENV" ]; then
    echo "--> Loading Geant4 environment..."
    source "$GEANT4_ENV"
fi

mkdir -p "$TEMP_BUILD_DIR"

cd "$TEMP_BUILD_DIR"
echo "--> Running CMake..."
cmake -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" "${PROJECT_ROOT}"

echo "--> Compiling with ${NUM_CORES} threads..."
make -j"${NUM_CORES}"

echo "--> Copying C++ binary back to project..."
if [ -f "${BINARY_NAME}" ]; then
    cp -f "${BINARY_NAME}" "${PROJECT_ROOT}/"
else
    echo "ERROR: Compiled binary not found!" >&2
    exit 1
fi

echo "=== C++ COMPILATION COMPLETED SUCCESSFULLY ==="
