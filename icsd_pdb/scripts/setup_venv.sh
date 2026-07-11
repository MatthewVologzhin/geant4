#!/bin/bash
# ==============================================================================
# setup_venv.sh
# Manages the Python virtual environment directly in the project directory.
# Installs and updates only primary dependencies defined in requirements.txt.
# ==============================================================================

set -e # Exit immediately on error

# Resolve paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Source the unified global configuration (located one level up)
if [ -f "${PROJECT_ROOT}/project_config.sh" ]; then
    source "${PROJECT_ROOT}/project_config.sh"
else
    echo "ERROR: project_config.sh not found in ${PROJECT_ROOT}" >&2
    exit 1
fi

echo "=== PYTHON ENVIRONMENT DEPLOYMENT (LOCAL PROJECT DIRECTORY) ==="
echo "Project Name:    ${PROJECT_NAME}"
echo "Local Venv:      ${LOCAL_VENV_DIR}"

# 1. Determine if venv needs compilation/update
NEEDS_COMPILE=false
if [ ! -d "$LOCAL_VENV_DIR" ] || [ ! -f "$ACTIVATE_SCRIPT" ]; then
    echo "--> Local venv not found. Initialization required."
    NEEDS_COMPILE=true
elif [ -f "$REQ_FILE" ] && [ -n "$(find "$REQ_FILE" -type f -newer "$ACTIVATE_SCRIPT" 2>/dev/null)" ]; then
    echo "--> requirements.txt has been updated. Update required."
    NEEDS_COMPILE=true
fi

# 2. Build or update python venv directly in the project folder
if [ "$NEEDS_COMPILE" = true ]; then
    if [ ! -d "$LOCAL_VENV_DIR" ]; then
        echo "--> Creating virtual environment in ${LOCAL_VENV_DIR}..."
        python3 -m venv "$LOCAL_VENV_DIR"
    fi
    
    echo "--> Activating environment..."
    source "$ACTIVATE_SCRIPT"
    
    echo "--> Upgrading pip..."
    pip install --upgrade pip
    
    echo "--> Installing Python dependencies from ${REQ_FILE}..."
    if [ -f "$REQ_FILE" ]; then
        pip install -r "$REQ_FILE"
    fi
    
    # Mark successful installation and update timestamps
    touch "${LOCAL_VENV_DIR}/.requirements_installed"
    touch "$ACTIVATE_SCRIPT"
    echo "--> Virtual environment is ready to use."
else
    echo "--> Virtual environment is up-to-date. Skipping installation."
fi