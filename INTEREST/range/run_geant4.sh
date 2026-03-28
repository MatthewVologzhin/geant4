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
THREADS=$9
MEMORY=${10}
JOB_ID=${11}
MACRO_NAME="run_${JOB_ID}.mac"
cat > ${MACRO_NAME} <<EOF
/run/verbose 0
/control/verbose 0
/run/numberOfThreads ${THREADS}
/range/det/setMat G4_WATER
/range/det/setRadius 10 m
/range/det/setTrackingCut 10 eV
/range/phys/addPhysics ${PHYSICS}
/run/setCut 10 m
/run/initialize
EOF

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
chmod +x range
./range ${MACRO_NAME}

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

#echo "=== Список .root файлов ==="
#ls -l root/*.root
# СРАЗУ ПОСЛЕ РАСЧЕТА: перемещаем все новые .root файлы в папку root
#mv *.root root/ 2>/dev/null || true
#echo "=== Список .root файлов ==="
#ls -l root/*.root
# Удаление временного макроса
rm ${MACRO_NAME}