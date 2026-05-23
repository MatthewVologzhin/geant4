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
THREADS=${10}
MEMORY=${11}
DISK=${12}
JOB_ID=${13}
MACRO_NAME="run_${JOB_ID}.mac"
cat > ${MACRO_NAME} << EOF
/run/verbose 0
/control/verbose 0
/tracking/verbose 0
/icsd/setGeom ${GEOMETRY}
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
./icsd ${MACRO_NAME}

rm ${MACRO_NAME}