#!/bin/bash
PHYSICS=$1
PART_TYPE=$2
ION_Z=$3
ION_A=$4
ION_Q=$5
ENERGY=$6
ENERGY_UNIT=$7
BEAM_ON=$8
RADIUS=$9
RADIUS_UNIT=${10}
LENGTH=${11}
LENGTH_UNIT=${12}
CYL_NUMBER=${13}
LOGX=${14}
THREADS=${15}
MEMORY=${16}
DISK=${17}
JOB_ID=${18}
MACRO_NAME="run_${JOB_ID}.mac"
cat > ${MACRO_NAME} << EOF
/run/verbose 0
/control/verbose 0
/tracking/verbose 0
/run/numberOfThreads ${THREADS}
/radial/setWorldRadius ${RADIUS} ${RADIUS_UNIT}
/radial/setCylinderLength ${LENGTH} ${LENGTH_UNIT}
/radial/setWorldOffsetLength 0. um
/radial/setCylinderNumber ${CYL_NUMBER}
/radial/setLog ${LOGX}

/run/setCut 1 nm
/cuts/setLowEdge 100 eV
/process/em/lowestMuHadEnergy 20 eV
/process/em/lowestElectronEnergy 20 eV
/process/eLoss/StepFunctionIons 0.1 10 nm

/process/em/fluo true
/process/em/auger true
/process/em/augerCascade true
/process/em/deexcitationIgnoreCut true
/radial/addPhysics ${PHYSICS}
EOF

if [ "${ION_A}" -gt 4 ]; then
    cat >> ${MACRO_NAME} << EOF
/radial/addIonsTrackingCut false
EOF
fi

cat >> ${MACRO_NAME} << EOF
/run/initialize
/gun/particle ${PART_TYPE}
EOF
if [ "${PART_TYPE}" == "ion" ]; then
    cat >> ${MACRO_NAME} << EOF
/gun/ion ${ION_Z} ${ION_A} ${ION_Q}
EOF
fi

if [ "${PHYSICS}" != "G4EmStandardPhysics_option4" ]; then
    cat >> ${MACRO_NAME} << EOF
/process/inactivate proton_G4DNAExcitation
/process/inactivate proton_G4DNAElastic
/process/inactivate proton_G4DNAChargeDecrease
/process/inactivate alpha_G4DNAExcitation
/process/inactivate alpha_G4DNAElastic
/process/inactivate alpha_G4DNAChargeDecrease
/process/inactivate e-_G4DNAElastic
/process/inactivate e-_G4DNAElectronSolvation
EOF
fi
if [[ "${PHYSICS}" != "G4EmStandardPhysics_option4" && "${PHYSICS}" != "G4EmDNAPhysics_option4" && "${PHYSICS}" != "G4EmDNAPhysics_option6" ]]; then
    cat >> ${MACRO_NAME} << EOF
/process/inactivate e-_G4DNAVibExcitation
/process/inactivate e-_G4DNAAttachment
EOF
fi

cat >> ${MACRO_NAME} << EOF
/process/inactivate msc
/process/inactivate hIoni
/process/inactivate nuclearStopping

/gun/energy ${ENERGY} ${ENERGY_UNIT}
/gun/direction 0 0 1
/run/beamOn ${BEAM_ON}
EOF

source /home/users/mvologzhin/programms/geant4-11.4-ref02/install/bin/geant4.sh
mkdir -p root
chmod +x radial
./radial ${MACRO_NAME}

rm ${MACRO_NAME}