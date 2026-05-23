#!/bin/bash
mkdir -p results/logs
mkdir -p results/output
mkdir -p results/data
mkdir -p results/plots

echo "Preparations are completed!"

condor_submit .sub

condor_q
echo "Calculations are completed!"