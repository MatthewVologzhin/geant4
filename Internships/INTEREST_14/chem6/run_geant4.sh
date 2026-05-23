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
THREADS=${12}
MEMORY=${13}
DISK=${14}
JOB_ID=${15}
MACRO_NAME="run_${JOB_ID}.mac"

if [ "$PART_TYPE" == "ion" ]; then
if [ "$MI" == "true" ]; then
FILENAME="chem6_${PART_TYPE}-${ION_A}_${ENERGY_VAL}-${ENERGY_UNIT}_${PHYSICS}_${CHEMISTRY}_${CHEMISTRY_METHOD}_MI_${JOB_ID}"
else
FILENAME="chem6_${PART_TYPE}-${ION_A}_${ENERGY_VAL}-${ENERGY_UNIT}_${PHYSICS}_${CHEMISTRY}_${CHEMISTRY_METHOD}_${JOB_ID}"
fi
else
if [ "$MI" == "true" ]; then
FILENAME="chem6_${PART_TYPE}_${ENERGY_VAL}-${ENERGY_UNIT}_${PHYSICS}_${CHEMISTRY}_${CHEMISTRY_METHOD}_MI_${JOB_ID}"
else
FILENAME="chem6_${PART_TYPE}_${ENERGY_VAL}-${ENERGY_UNIT}_${PHYSICS}_${CHEMISTRY}_${CHEMISTRY_METHOD}_${JOB_ID}"
fi
fi


mkdir -p output

printf "# === 1. ФИЗИКА И ХИМИЯ ===
/chem/phys/registerPhysics ${PHYSICS}
/chem/phys/registerPhysics ${CHEMISTRY}
/process/chem/TimeStepModel ${CHEMISTRY_METHOD}
/chem/phys/PhysChemVerbose 1
/run/numberOfThreads ${THREADS}
# Включаем MI, если передано true. Если MI=false, Oxy просто не будет рождаться.
/process/dna/UseDNAMultipleIonisation ${MI}
/process/dna/e-SolvationSubType Meesungnoen2002

/run/initialize

# === 3. ХИМИЧЕСКИЕ РЕАКЦИИ (UserID из твоего grep) ===
/chem/reaction/print
/chem/PrintSpeciesTable

# === 4. ГЕОМЕТРИЯ И УСЛОВИЯ ОСТАНОВКИ ===
/primaryKiller/setSize 10 10 10 mm
/gun/position  0 0 -5 mm
/gun/direction 0 0 1
/primaryKiller/eLossMin 10 keV
/primaryKiller/eLossMax 10.1 keV

# === 5. СКОРЕРЫ ===
/scorer/LET/cutoff 1 GeV
/scorer/species/fileName output/${FILENAME}_
/scorer/species/nOfTimeBins 60
/scheduler/endTime 1 microsecond

/run/printProgress 1
/gun/particle ${PART_TYPE}
" > ${MACRO_NAME}

if [ "$PART_TYPE" == "ion" ]; then
cat >> ${MACRO_NAME} << EOF
/gun/ion ${ION_Z} ${ION_A} ${ION_Q}
EOF
fi

CLEAN_ID=$(echo ${JOB_ID} | tr -d '_')
cat >> ${MACRO_NAME} << EOF
/gun/energy ${ENERGY_VAL} ${ENERGY_UNIT}
/random/setSeeds ${CLEAN_ID} ${CLEAN_ID}
/run/beamOn ${BEAM_ON}
EOF

source /home/users/mvologzhin/programms/geant4-11.4-ref02/install/bin/geant4.sh
chmod +x chem6
./chem6 ${MACRO_NAME}

if [ -f "Species.txt" ]; then
    mv Species.txt output/${FILENAME}_LET.txt
fi

rm ${MACRO_NAME}