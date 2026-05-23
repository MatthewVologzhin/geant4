#!/bin/bash

echo "---=== ROOT Files | Start processing ===---"
mkdir -p results/plots

root -l -b -q analysis/plotG_LET.C

STATUS=$?
if [ $STATUS -ne 0 ]; then
    echo "ERROR: Plotting failed with exit code $STATUS"
    exit $STATUS
fi

echo "Plotting finished successfully. See results/plots/Validation_All_States.pdf"