#!/bin/bash
# scripts/run_merge.sh

INPUT_DIR="results/output"
DATA_DIR="results/data"

mkdir -p "$DATA_DIR"

# Массивы параметров
PARTICLES=("proton" "alpha" "ion-12" "ion-20")
PHYS_LISTS=("G4EmDNAPhysics_option2" "G4EmDNAPhysics_option4" "G4EmDNAPhysics_option6" "G4EmDNAPhysics_option8")
CHEM_LISTS=("G4EmDNAChemistry" "G4EmDNAChemistry_option1" "G4EmDNAChemistry_option2" "G4EmDNAChemistry_option3")
CHEM_METHODS=("SBS" "IRT" "IRT_syn")

# 4 новых состояния мультиионизации
MI_STATES=("MI_P-OFF_C-OFF" "MI_P-ON_C-OFF" "MI_P-OFF_C-ON" "MI_P-ON_C-ON")

for part in "${PARTICLES[@]}"; do
    for phys in "${PHYS_LISTS[@]}"; do
        for chem in "${CHEM_LISTS[@]}"; do
            for method in "${CHEM_METHODS[@]}"; do
                for mi in "${MI_STATES[@]}"; do
                    
                    # Имя итогового файла
                    target_file="${DATA_DIR}/${part}_${phys}_${chem}_${method}_${mi}.txt"

                    # 1. Формируем строгий паттерн для поиска файлов
                    # Благодаря жестким подчеркиваниям "_", паттерн "*_${method}_${mi}_*" 
                    # автоматически исключает путаницу между "IRT" и "IRT_syn",
                    # поэтому "grep -v" больше не нужен!
                    PATTERN="${INPUT_DIR}/chem6_${part}_*_${phys}_${chem}_${method}_${mi}_*_LET.txt"
                    
                    # 2. Находим все файлы по паттерну, подавляя ошибку, если файлов нет
                    FILES=$(ls $PATTERN 2>/dev/null)

                    # 3. Мерджим, если что-то нашли
                    if [ -n "$FILES" ]; then
                        echo "Merging: $part | $phys | $chem | $method | $mi"
                        > "$target_file"
                        for f in $FILES; do
                            if [ -f "$f" ]; then
                                cat "$f" >> "$target_file"
                                echo "" >> "$target_file" # Добавляем перенос строки между выводами
                            fi
                        done
                    fi

                    # Если итоговый файл пустой (не было данных), удаляем его, чтобы не плодить мусор
                    if [ ! -s "$target_file" ]; then 
                        rm -f "$target_file"
                    fi
                done
            done
        done
    done
done

echo "Merging done!"