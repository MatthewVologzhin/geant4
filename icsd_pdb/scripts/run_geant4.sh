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
GEOMETRY_METHOD=${10}
VOXEL_VAL=${11}
VOXEL_UNIT=${12}
THREADS=${13}
MEMORY=${14}
DISK=${15}
JOB_ID=${16}
MACRO_NAME="run_${JOB_ID}.mac"

ENERGY_VAL_P="${ENERGY_VAL//./p}"
if [ "${PART_TYPE}" == "ion" ]; then
    if [ "${VOXEL_VAL}" == 0 ]; then
        FILENAME="output/root/icsd_${PART_TYPE}-${ION_Z}-${ION_A}_${ENERGY_VAL_P}-${ENERGY_UNIT}_${PHYSICS}_${GEOMETRY}_${GEOMETRY_METHOD}_${JOB_ID}"
    else
        FILENAME="output/root/icsd_${PART_TYPE}-${ION_Z}-${ION_A}_${ENERGY_VAL_P}-${ENERGY_UNIT}_${PHYSICS}_${GEOMETRY}_${GEOMETRY_METHOD}_${VOXEL_VAL}-${VOXEL_UNIT}_${JOB_ID}"
    fi
else
    if [ "${VOXEL_VAL}" == 0 ]; then
        FILENAME="output/root/icsd_${PART_TYPE}_${ENERGY_VAL_P}-${ENERGY_UNIT}_${PHYSICS}_${GEOMETRY}_${GEOMETRY_METHOD}_${JOB_ID}"
    else
        FILENAME="output/root/icsd_${PART_TYPE}_${ENERGY_VAL_P}-${ENERGY_UNIT}_${PHYSICS}_${GEOMETRY}_${GEOMETRY_METHOD}_${VOXEL_VAL}-${VOXEL_UNIT}_${JOB_ID}"
    fi
fi
cat > ${MACRO_NAME} << EOF
/run/verbose 2
/control/verbose 0
/tracking/verbose 0
/run/numberOfThreads ${THREADS}
/run/printProgress 100
/icsd/analysis/setFileName ${FILENAME}
/det/setGeom ${GEOMETRY}
/det/setGeomMethod ${GEOMETRY_METHOD}
/det/setDataDir output/csv
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

source ${GEANT4_ENV}
chmod +x icsd
mkdir -p output/root
mkdir -p output/csv

# HTCondor transfers directories using their base name (creating "csv" in root),
# or flattens files directly into the root workspace. We handle both scenarios here.
if [ -d "csv" ]; then
    # Scenario A: "csv" directory exists at root. Move it into "output" container.
    mkdir -p output
    mv csv output/
elif ls *.csv 1>/dev/null 2>&1; then
    # Scenario B: Files were flattened into the root workspace. Create output/csv and move them.
    mkdir -p output/csv
    for csv_file in *.csv; do
        if [ -f "$csv_file" ] && [ "$csv_file" != "geometry_benchmark.csv" ]; then
            mv "$csv_file" output/csv/
        fi
    done
fi

./icsd ${MACRO_NAME}

if [ -f "geometry_benchmark.csv" ]; then
    mv geometry_benchmark.csv "output/root/geometry_benchmark_${JOB_ID}.csv"
fi

rm ${MACRO_NAME}
