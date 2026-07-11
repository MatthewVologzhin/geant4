# ==============================================================================
# project_config.sh
# GLOBAL CONFIGURATION FILE - MODIFY ONLY THIS FILE TO PORT TO OTHER PROJECTS!
# ==============================================================================

# ------------------------------------------------------------------------------
# 1. USER-DEFINED SETTINGS (Modify these when reusing scripts for other projects)
# ------------------------------------------------------------------------------
PROJECT_NAME="icsd_pdb"                               # Project name/identifier
BINARY_NAME="icsd"                                    # C++ executable target name
BUILD_TYPE="Release"                                  # CMAKE_BUILD_TYPE (Release / Debug)
export GEANT4_ENV="/home/users/mvologzhin/programms/geant4-dev/install/bin/geant4.sh" # Path to Geant4 env

# ------------------------------------------------------------------------------
# 2. AUTOMATIC PATH RESOLUTION (Do not modify unless changing folder layout)
# ------------------------------------------------------------------------------
# Dynamically find the directory containing this config file (project root)
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Local project folders
LOCAL_VENV_DIR="${PROJECT_ROOT}/.venv"
REQ_FILE="${PROJECT_ROOT}/requirements.txt"
ACTIVATE_SCRIPT="${LOCAL_VENV_DIR}/bin/activate"

# Fast local SSD temporary folder for C++ compilation only
TEMP_BUILD_DIR="/tmp/${USER}_build_${PROJECT_NAME}"