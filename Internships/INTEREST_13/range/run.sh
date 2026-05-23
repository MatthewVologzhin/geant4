#!/bin/bash
mkdir -p logs
mkdir -p root

echo "Подготовка завершена. Скрипт отправлен на HTCondor"

condor_submit submit.sub

condor_q

echo "Готово!"
