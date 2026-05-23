#!/bin/bash
mkdir -p logs
mkdir -p output
echo "Preparations are completed!"

condor_submit submit.sub

condor_q
echo "Calculations are runnig!"