#!/bin/bash
mkdir -p results/logs
mkdir -p results/output
mkdir -p results/csv
mkdir -p results/plots

echo "Preparations are completed!"

condor_submit jobs/geant4.sub

watch -n 2 condor_q
echo "Calculations are completed!"
