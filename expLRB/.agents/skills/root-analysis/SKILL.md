---
name: root-analysis
description: Обработка, аудит и визуализация .root файлов (BioData TTree, ICSD, SpecIn_*) через ROOT C++/ACLiC (plot.C), а также объединение серий через hadd.
---

# Навык: ROOT-анализ и визуализация (plot.C / hadd)

## Строгие правила анализа
1. **Никаких подменок и fallback'ов:**
   - **Обязательные (core) ветки** `BioData` присутствуют в любом скоринг-профиле: `EventID`, `Edep_Total`, `Edep_EM`, `Edep_HadTotal`, `Ions_Total`, `Ions_EM`, `Ions_HadTotal`, `HitTargetCount`. Их отсутствие — явная ошибка, сценарий не грузится.
   - **Опциональные группы** (`Edep_/Ions_*` по species/nonDNA/sub-channels, гистограммы `ICSD_*`/`SpecIn_*`) читаются **только если активный профиль их записал** (проверка `tree->GetBranch(...)`). Отсутствие — это не ошибка: соответствующие графики пропускаются, а не подменяются дефолтом.
   - Оси графиков вычисляются автоматически по непустым данным из `.root` файла либо берутся из `config/analysis.conf`.
2. **Визуальный стандарт (стиль `plotICSDValidation.C`):**
   - Холст `1920x1080` (`2400x1600` для сетки `3x3`), формат `.svg`, `TH1::AddDirectory(kFALSE)`.
   - Заголовок — в верхнем поле (`topMargin = 0.09`), запас снизу (`bottomMargin = 0.15`, `TitleOffset >= 1.15`).
   - Двойная легенда с зазором между текстом и иконками (`x2 - iconWidth - gap`).
   - Исключать нулевые бины при построении `TGraphAsymmErrors` на `Logy`-графиках (без вертикальных zero-drop провалов).
   - У графиков `dN / d(ln E)` нормировать содержимое бина на `ln(E_high / E_low)` и на число событий `nEvents`.
3. **Инспекция `.root` и запуск:**
   - Быстрая проверка состава `.root`-файла (короткий one-liner, без чтения крупных файлов целиком):
     `root -l -b -q -e 'TFile f("out.root"); f.ls(); ((TTree*)f.Get("BioData"))->Print();'`
   - Запуск анализа — **из корня проекта** (`#include "config/plot_scenarios.inc"` резолвится относительно `plot.C`):
     `root -l -q 'plot.C+'`
   - Список сценариев для анализа — `config/plot_scenarios.inc` (генерируется из `config/runs.conf`); косметика — `config/analysis.conf`.
   - Отсутствующие входные `.root` пропускаются с `[ERROR]`; если не загрузился ни один сценарий — жёсткая остановка (`[FATAL] No scenarios could be loaded`).
4. **Математика `hadd`:**
   - Объединять через `hadd` можно **только** файлы с независимыми сидами ГПСЧ.
   - Средние (`Mean M1`, `Dose Q`, `Dose Share %`, `F(>=k)`) всегда считать **после** `hadd` по суммарным интегралам.
   - Объединение чанков одного сценария: `scripts/merge_all.sh <scenario> output/merged output/raw`.
