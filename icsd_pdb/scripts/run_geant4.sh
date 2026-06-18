#!/bin/bash
PHYSICS=$1
PART_TYPE=$2
ION_Z=$3
ION_A=$4
ION_Q=$5
ENERGY_VAL=$6
ENERGY_UNIT=$7
BEAM_ON=$8
GEOMETRY=$9
VOXEL_VAL=${10}
VOXEL_UNIT=${11}
THREADS=${12}
MEMORY=${13}
DISK=${14}
JOB_ID=${15}
MACRO_NAME="run_${JOB_ID}.mac"

ENERGY_VAL_P="${ENERGY_VAL//./p}"
if [ "${PART_TYPE}" == "ion" ]; then
    if [ "${VOXEL_VAL}" == 0 ]; then
        FILENAME="results/output/icsd_${PART_TYPE}-${ION_Z}-${ION_A}_${ENERGY_VAL_P}-${ENERGY_UNIT}_${PHYSICS}_${GEOMETRY}_${JOB_ID}"
    else
        FILENAME="results/output/icsd_${PART_TYPE}-${ION_Z}-${ION_A}_${ENERGY_VAL_P}-${ENERGY_UNIT}_${PHYSICS}_${GEOMETRY}_${VOXEL_VAL}-${VOXEL_UNIT}_${JOB_ID}"
    fi
else
    if [ "${VOXEL_VAL}" == 0 ]; then
        FILENAME="results/output/icsd_${PART_TYPE}_${ENERGY_VAL_P}-${ENERGY_UNIT}_${PHYSICS}_${GEOMETRY}_${JOB_ID}"
    else
        FILENAME="results/output/icsd_${PART_TYPE}_${ENERGY_VAL_P}-${ENERGY_UNIT}_${PHYSICS}_${GEOMETRY}_${VOXEL_VAL}-${VOXEL_UNIT}_${JOB_ID}"
    fi
fi
cat > ${MACRO_NAME} << EOF
/run/verbose 1
/control/verbose 0
/tracking/verbose 0
/run/numberOfThreads ${THREADS}
/icsd/analysis/setFileName ${FILENAME}
/icsd/setGeom ${GEOMETRY}
EOF

if [ ${VOXEL_VAL} != "0" ]; then
cat >> ${MACRO_NAME} << EOF
/det/setVoxelSize ${VOXEL_VAL} ${VOXEL_UNIT}
EOF
fi

cat >> ${MACRO_NAME} << EOF
/physics/setPhysics ${PHYSICS}
/run/initialize
EOF

cat >> ${MACRO_NAME} << EOF
/gun/particle ${PART_TYPE}
EOF
if [ "${PART_TYPE}" == "ion" ]; then
    cat >> ${MACRO_NAME} << EOF
    /gun/ion ${ION_Z} ${ION_A} ${ION_Q}
EOF
fi
cat >> ${MACRO_NAME} << EOF
/gun/energy ${ENERGY_VAL} ${ENERGY_UNIT}
/run/beamOn ${BEAM_ON}
EOF

source /home/users/mvologzhin/programms/geant4-dev/install/bin/geant4.sh
chmod +x icsd
mkdir -p results/output
./icsd ${MACRO_NAME}

rm ${MACRO_NAME}
