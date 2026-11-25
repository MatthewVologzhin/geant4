#!/usr/bin/env bash
set -e

# Определение рабочей директории
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" && pwd)"

# 1. Подгрузка project_config.sh
if [ -f "${SCRIPT_DIR}/project_config.sh" ]; then
    source "${SCRIPT_DIR}/project_config.sh"
elif [ -f "${SCRIPT_DIR}/../project_config.sh" ]; then
    source "${SCRIPT_DIR}/../project_config.sh"
elif [ -f "${SCRIPT_DIR}/../../project_config.sh" ]; then
    source "${SCRIPT_DIR}/../../project_config.sh"
else
    echo "ERROR: project_config.sh not found!" >&2
    exit 1
fi

# 2. Инициализация Geant4 окружения
if [ -n "${GEANT4_ENV}" ] && [ -f "${GEANT4_ENV}" ]; then
    source "${GEANT4_ENV}"
else
    echo "ERROR: Geant4 environment file not found at: '${GEANT4_ENV}'" >&2
    exit 1
fi

# 3. Настройка быстрой сборки в /tmp/
if [ -z "${TEMP_BUILD_DIR}" ]; then
    TEMP_BUILD_DIR="/tmp/${USER}_build_${PROJECT_NAME:-chem6_plots}"
fi

mkdir -p "${TEMP_BUILD_DIR}"

CPP_SOURCE="${PROJECT_ROOT:-$SCRIPT_DIR}/scripts/plot/plotGvsLET.cpp"
BUILD_BIN="${TEMP_BUILD_DIR}/plot_g_let"

# 4. Компиляция в /tmp/ (если бинарник отсутствует или исходник обновлен)
if [ ! -f "${BUILD_BIN}" ] || [ "${CPP_SOURCE}" -nt "${BUILD_BIN}" ]; then
    echo "[*] Compiling ${CPP_SOURCE} into fast /tmp/ build directory..."
    
    G4_LIB_DIR="$(geant4-config --prefix)/lib"
    [ ! -d "$G4_LIB_DIR" ] && G4_LIB_DIR="$(geant4-config --prefix)/lib64"

    g++ -O3 -std=c++17 "${CPP_SOURCE}" \
        $(root-config --cflags --libs) \
        $(geant4-config --cflags --libs) \
        -Wl,-rpath,"${G4_LIB_DIR}" \
        -o "${BUILD_BIN}"

    echo "[+] Compiled successfully: ${BUILD_BIN}"
fi

# 5. Запуск из корня проекта
cd "${PROJECT_ROOT:-$SCRIPT_DIR}"
"${BUILD_BIN}" "$@"