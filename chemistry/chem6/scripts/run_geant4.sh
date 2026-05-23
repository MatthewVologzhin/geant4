#!/bin/bash
PHYSICS=$1
CHEMISTRY=$2
CHEMISTRY_METHOD=$3
MI_PHYS=$4    # true или false
MI_CHEM=$5    # true или false
PART_TYPE=$6
ION_Z=$7
ION_A=$8
ION_Q=$9
ENERGY_VAL=${10}
ENERGY_UNIT=${11}
BEAM_ON=${12}
THREADS=${13}
MEMORY=${14}
DISK=${15}
JOB_ID=${16}
MACRO_NAME="run_${JOB_ID}.mac"
if [ "$PART_TYPE" == "ion" ]; then
    PART_STR="${PART_TYPE}-${ION_A}"
    # Для тяжелых ионов ЛПЭ огромная. Шаг может быть большим.
    # ELOSS_MIN=30   # 30 кэВ достаточно для отличного трека
    # ELOSS_MAX=150  # Очень широкое окно, чтобы тяжелый ион не вылетел за пределы (aborted)
    # BEAM_ON=50     # 50 событий хватит за глаза для отличной статистики
elif [ "$PART_TYPE" == "alpha" ]; then
    PART_STR="${PART_TYPE}"
    # ELOSS_MIN=15
    # ELOSS_MAX=60   # Широкое окно для дельта-электронов
    #BEAM_ON=100    # 100 событий
else
    PART_STR="${PART_TYPE}" # протоны
    # ELOSS_MIN=10
    # ELOSS_MAX=30
    #BEAM_ON=200
fi

# 2. Формируем короткие метки для состояний MI (ON/OFF для красоты имени файла)
[ "$MI_PHYS" == "true" ] && P_LBL="ON" || P_LBL="OFF"
[ "$MI_CHEM" == "true" ] && C_LBL="ON" || C_LBL="OFF"
MI_STR="MI_P-${P_LBL}_C-${C_LBL}"

# 3. Собираем итоговое имя файла в одну строчку
FILENAME="chem6_${PART_STR}_${ENERGY_VAL}-${ENERGY_UNIT}_${PHYSICS}_${CHEMISTRY}_${CHEMISTRY_METHOD}_${MI_STR}_${JOB_ID}"


mkdir -p output

printf "# === 1. ФИЗИКА И ХИМИЯ ===
/run/numberOfThreads ${THREADS}
/run/verbose 1
/tracking/verbose 0
/event/verbose 0
/chem/phys/PhysChemVerbose 0

/chem/phys/registerPhysics ${PHYSICS}
#/chem/phys/registerPhysics ${CHEMISTRY}
/process/chem/TimeStepModel ${CHEMISTRY_METHOD}
/process/dna/UseDNAMultipleIonisation ${MI_PHYS}
/process/dna/e-SolvationSubType Meesungnoen2002

# Регистрация ионов MI (имена должны соответствовать тем, что генерирует физика)
#/chem/species H2O^2 [ H2O | 2 | 2e-9 | 0.275 ]
#/chem/species H2O^3 [ H2O | 3 | 2e-9 | 0.275 ]
#/chem/species H2O^4 [ H2O | 4 | 2e-9 | 0.275 ]

/run/initialize

# /chem/reaction/UI
" > ${MACRO_NAME}

#if [ "$MI_CHEM" == "true" ]; then
#cat >> ${MACRO_NAME} << EOF
# === Таблица 14.3: Диссоциация MI-состояний ===
#/chem/reaction/add H2O^2 + H2O(B) -> H3Op + H3Op + Oxy | Fix | 1.0e15 | 0
#/chem/reaction/add H2O^3 + H2O(B) -> H3Op + H3Op + H3Op + HO2° | Fix | 1.0e15 | 0
#/chem/reaction/add H2O^4 + H2O(B) -> H3Op + H3Op + H3Op + H3Op + O2 | Fix | 1.0e15 | 0
#EOF
#else
#cat >> ${MACRO_NAME} << EOF
#/chem/reaction/add H2O^2 + H2O(B) -> H3Op + °OH | Fix | 1.0e15 | 0
#/chem/reaction/add H2O^3 + H2O(B) -> H3Op + °OH | Fix | 1.0e15 | 0
#/chem/reaction/add H2O^4 + H2O(B) -> H3Op + °OH | Fix | 1.0e15 | 0
#EOF
#fi

printf "
# === 3. ХИМИЧЕСКИЕ РЕАКЦИИ (UserID из твоего grep) ===
/chem/reaction/print
/chem/PrintSpeciesTable

# === 4. ГЕОМЕТРИЯ И УСЛОВИЯ ОСТАНОВКИ ===
/primaryKiller/setSize 2 2 2 um
/gun/position  0 0 -0.99 um
/gun/direction 0 0 1
# /primaryKiller/eLossMin ${ELOSS_MIN} keV
# /primaryKiller/eLossMax ${ELOSS_MAX} keV

# === 5. СКОРЕРЫ ===
/scorer/LET/cutoff 1 GeV
/scorer/species/fileName output/${FILENAME}_
/scorer/species/nOfTimeBins 60
/scheduler/endTime 1 microsecond

/run/printProgress 1
/gun/particle ${PART_TYPE}
" >> ${MACRO_NAME}

if [ "$PART_TYPE" == "ion" ]; then
cat >> ${MACRO_NAME} << EOF
/gun/ion ${ION_Z} ${ION_A} ${ION_Q}
EOF
fi

CLEAN_ID=$(echo ${JOB_ID} | tr -d '_')
cat >> ${MACRO_NAME} << EOF
/gun/energy ${ENERGY_VAL} ${ENERGY_UNIT}
/run/beamOn ${BEAM_ON}
EOF

#source /home/users/mvologzhin/programms/geant4-11.4-ref02/install/bin/geant4.sh
source /home/users/mvologzhin/programms/geant4-dev/install/bin/geant4.sh
chmod +x chem6
./chem6 ${MACRO_NAME}
STATUS=$?

if [ -f "Species.txt" ]; then
    mv Species.txt output/${FILENAME}_LET.txt
fi

rm ${MACRO_NAME}

if [ $STATUS -ne 0 ]; then
    echo "ERROR: chem6 failed with exit code $STATUS"
    exit $STATUS
fi