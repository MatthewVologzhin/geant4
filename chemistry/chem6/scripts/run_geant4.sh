#!/bin/bash
PHYSICS=$1
CHEMISTRY=$2
CHEMISTRY_METHOD=$3
MI=$4
PART_TYPE=$5
ION_Z=$6
ION_A=$7
ION_Q=$8
ENERGY_VAL=$9
ENERGY_UNIT=${10}
BEAM_ON=${11}
ELOSS_MIN=${12}
ELOSS_MAX=${13}
THREADS=${14}
MEMORY=${15}
DISK=${16}
JOB_ID=${17}
MACRO_NAME="run_${JOB_ID}.mac"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [ -f "${SCRIPT_DIR}/../project_config.sh" ]; then
    source "${SCRIPT_DIR}/../project_config.sh"
fi

[ "$MI" == "true" ] && MI_LBL="ON" || MI_LBL="OFF"
MI_STR="MI-${MI_LBL}"
PART_STR="ion-${ION_Z}-${ION_Q}"
FILENAME="chem6_${PART_STR}_${ENERGY_VAL}-${ENERGY_UNIT}_${PHYSICS}_${CHEMISTRY}_${CHEMISTRY_METHOD}_${MI_STR}_${JOB_ID}"

mkdir -p output/data

printf "# === 1. ФИЗИКА И ХИМИЯ ===
/run/numberOfThreads ${THREADS}

/chem/phys/registerPhysics ${PHYSICS}
/chem/phys/registerPhysics ${CHEMISTRY}
/process/chem/TimeStepModel ${CHEMISTRY_METHOD}
/process/dna/UseDNAMultipleIonisation ${MI}
/process/dna/e-SolvationSubType Meesungnoen2002
/process/dna/ionMaxEnergyPerNucleon 1 GeV

/run/initialize

/scheduler/endTime 100 ns
/scorer/species/nOfTimeBins 50

/primaryKiller/setSize 1 1 1 mm
/gun/position 0 0 0 mm
/gun/direction 0 0 1

/tracking/verbose 0
/scheduler/verbose 0
/run/printProgress 100

/scorer/species/fileName output/data/${FILENAME}_

/gun/particle ${PART_TYPE}
" >> ${MACRO_NAME}

if [ "$PART_TYPE" == "ion" ]; then
cat >> ${MACRO_NAME} << EOF
/gun/ion ${ION_Z} ${ION_A} ${ION_Q}
EOF
fi

CLEAN_ID=$(echo ${JOB_ID} | tr -d '_')
cat >> ${MACRO_NAME} << EOF
/primaryKiller/eLossMin ${ELOSS_MIN} keV
/primaryKiller/eLossMax ${ELOSS_MAX} keV

/gun/energy ${ENERGY_VAL} ${ENERGY_UNIT}
/run/beamOn ${BEAM_ON}
EOF

# Активация Geant4
if [ -f "${GEANT4_ENV}" ]; then
    source "${GEANT4_ENV}"
else
    echo "ERROR: Geant4 environment file not found at ${GEANT4_ENV}" >&2
    exit 1
fi

# Запуск расчета
chmod +x chem6
./chem6 ${MACRO_NAME}
STATUS=$?

echo "Завершение расчета для задачи ${JOB_ID}. Переименование файлов..."

if [ -f "Species.txt" ]; then
    mv Species.txt output/data/${FILENAME}_LET.txt
fi

rm ${MACRO_NAME}

if [ $STATUS -ne 0 ]; then
    echo "ERROR: chem6 failed with exit code $STATUS"
    exit $STATUS
fi