#!/bin/bash
# Аргументы из HTCondor
PHYSICS=$1
PART_TYPE=$2
ION_Z=$3
ION_A=$4
ION_Q=$5
ENERGY_VAL=$6
ENERGY_UNIT=$7
BEAM_ON=$8
NUCLEAR=$9
THREADS=${10}
MEMORY=${11}
JOB_ID=${12}
MACRO_NAME="run_${JOB_ID}.mac"
cat > ${MACRO_NAME} <<EOF
/run/verbose 0
/control/verbose 0
/control/cout/ignoreThreadsExcept 0
/tracking/verbose 0
/run/numberOfThreads ${THREADS}
/spower/det/setMat G4_WATER
/spower/det/setRadius 1 m
/spower/phys/addPhysics ${PHYSICS}
/run/initialize
EOF

if [ "${NUCLEAR}" == "true" ]; then
    cat >> ${MACRO_NAME} << EOF
/process/inactivate nuclearStopping
EOF
fi


cat >> ${MACRO_NAME} <<EOF
/gun/particle ${PART_TYPE}
EOF
if [ "${PART_TYPE}" == "ion" ]; then
  cat >> ${MACRO_NAME} <<EOF
/gun/ion ${ION_Z} ${ION_A} ${ION_Q}
EOF
fi
cat >> ${MACRO_NAME} <<EOF
/gun/energy ${ENERGY_VAL} ${ENERGY_UNIT}
/run/beamOn ${BEAM_ON}
EOF
# Активация Geant4
source /home/users/mvologzhin/programms/geant4-11.4-ref02/install/bin/geant4.sh
# Воссоздаем структуру папок
mkdir -p root
# Запуск расчета
chmod +x spower
./spower ${MACRO_NAME}

echo "Завершение расчета для задачи ${JOB_ID}. Переименование файлов..."

# Заходим в папку с результатами
if [ -d "root" ]; then
    cd root
    # Проходим по всем файлам и добавляем к ним префикс ID задачи
    for file in *; do
        # Проверяем, что это файл и мы его еще не переименовали
        if [[ -f "$file" && ! "$file" == "${JOB_ID}_"* ]]; then
            mv "$file" "${JOB_ID}_${file}"
            echo "Файл $file переименован в ${JOB_ID}_${file}"
        fi
    done
    cd ..
fi

rm ${MACRO_NAME}