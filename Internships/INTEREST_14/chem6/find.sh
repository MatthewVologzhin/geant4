#!/bin/bash
SRC_DIR="/home/users/mvologzhin/programms/geant4-11.4-ref02/geant4-dev-master/source"

echo "=== 1. Ищем, как называются молекулы в конструкторе Chemistry Option 3 ==="
# Ищем вызовы CreateConfiguration, где задаются имена
grep -rn "CreateConfiguration" $SRC_DIR/physics_lists/constructors/electromagnetic/src/G4EmDNAChemistry_option3.cc

echo -e "\n=== 2. Ищем определения имен в классах молекул (например, Кислород) ==="
# Ищем, какую строку возвращает GetName() для разных определений
grep -rn "GetName" $SRC_DIR/processes/electromagnetic/dna/molecules/

echo -e "\n=== 3. Ищем, как прописан атомарный кислород для множественной ионизации ==="
grep -rn "G4Oxygen" $SRC_DIR/processes/electromagnetic/dna/molecules/