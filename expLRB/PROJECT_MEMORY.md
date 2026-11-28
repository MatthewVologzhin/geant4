# PROJECT_MEMORY.md — Архитектура, физика и реестр «граблей»

## 1. Физическая и геометрическая модель эксперимента (У-400М)
- **Пучок:** `20Ne`, `50 МэВ/нуклон` (`1.0 ГэВ` суммарно), апертура `14 × 14 мм²` (`setRadius 7 mm` в `PrimaryGeneratorAction` разыгрывает **квадрат** `[-7, +7] × [-7, +7] мм`, а не круг).
- **Тракт перед образцом (по Z):**
  1. `G4_Ta` (`4 мкм`)
  2. `G4_Fe` (`3 мкм`)
  3. Ионизационная камера: 3 электрода `G4_Al` по `5 мкм` с двумя зазорами `G4_AIR` по `6 мм`
  4. Опциональный дополнительный фильтр: `G4_Al` (`1.815 мм`)
  5. Воздушный зазор: `G4_AIR` (`10.0 мм`)
  6. Стенка пробирки: `G4_POLYETHYLENE` (`1.200 мм`, rho = `0.94 г/см³`)
- **Биологический образец (`BioDetectorConstruction`):**
  - Внешний бокс среды `RPMI_1640` (rho = `1.0145 г/см³`, 10 элементов), `14 × 14 × 0.015 мм` для монослоя (`186 000` клеток).
  - Клетка (3 концентрических `G4Orb`): ядро R = `3.1 мкм` (rho = `1.0`), цитоплазма толщиной `0.5 мкм` (rho = `1.0`), мембрана толщиной `0.01 мкм` (rho = `1.1`).
  - Команда `/bio/layer/create` принимает **2 аргумента**: `<material> <number_cell>` (прежний выбор `monolayer`/`suspension` удалён).
  - В `CellRegion` при включённой `DNA_Opt6` материалы клеток автоматически подменяются на масштабированную по плотности воду `G4_WATER_N/C/M` (необходимо для моделей Geant4-DNA).

## 2. Физические решения и настройка порогов (Cut'ы)
- **Базовая EM и DNA:** вне клеток — `EM_Opt4`, внутри `CellRegion` — `DNA_Opt6` (`CPA100`) через `G4EmDNAPhysicsActivator`.
- **Пороги электронов:**
  - В `PhysicsList::ConstructProcess()` принудительно выставляются `SetLowestElectronEnergy(7.4 * eV)` (иначе `EM_Opt4` глушит электроны на `100 eV` и занижает ICSD) и `G4ProductionCutsTable::SetEnergyRange(100 * eV, 100 * GeV)` (иначе таблица cut'ов зажимает минимум на `990 eV`).
  - `DefaultCut = 1 mm` (~350 кэВ для e-) полностью подавляет дельта-электроны от `50 MeV/u` `20Ne` (T_max ~ `113 кэВ`). Поэтому установлены: `PreBioRegion` (`10 мкм` на последнем слое тракта — полиэтилене), `BioRegion` (`0.5 мкм`), `CellRegion` (`10 нм`). Передние фильтры (`Ta, Fe, Al, Air`) остаются с `1 mm` cut, чтобы не замедлять счёт.
- **Подсчёт ионизаций (`BioSensitiveDetector::CalculateIonizations`):**
  - Формула `edep / fWvalue` удалена. Засчитывается строго `1` ионизация на дискретный шаг с `SubType == fLowEnergyIonisation` (в Geant4 11.4 это `53`), иначе `0`.
  - **Нюанс с дейтронами и тритонами:** `deuteron` и `triton` не имеют DNA-моделей (идут через `EM_Opt4` `G4hIonisation` с `SubType == fIonisation`), поэтому их прямые столкновения возвращают `0` в `CalculateIonizations`, а в `ICSD` попадают только их вторичные дельта-электроны. Требует аудита.
- **Входные спектры (`SpecIn_*`):**
  - Считаются прямо в `BioSensitiveDetector::ProcessHits` при `pre->GetStepStatus() == fGeomBoundary` **до** отсечки `if (edep <= 0.0)`, только для `HadronicCascade` с фильтрацией `e+-/gamma` (`|pdg|==11 || pdg==22`).
  - Для вложенных сферических оболочек (`Cytoplasm`, `WholeCell`) выход из внутренней сферы отсекается в локальной системе клетки проверкой `localPos.dot(localDir) < 0`; для `Nucleus` дочерних объёмов нет.

## 3. Критические «грабли» Geant4 11.4, которые уже пойманы
1. **`ConstructParticle()` вызывается ДО макроса:** `G4MTRunManager::SetUserInitialization(new PhysicsList())` в `main.cc` сразу вызывает `ConstructParticle()` в `PreInit`. Любая финализация процессов и флагов допустима **только** в `ConstructProcess()` (при `/run/initialize`), а UI-методы в `PreInit` проверяются через `G4StateManager::GetCurrentState() == G4State_PreInit`.
2. **Запрет конструкторов EM в `/run/initialize`:** конструктор `G4EmStandardPhysics_option4` вызывает `G4EmParameters::SetDefaults()`, стирая `AddDNA(...)`. Поэтому замена EM происходит только в `PreInit` через `ReplacePhysics(...)`.
3. **Имена конструкторов в `G4PhysicsConstructorRegistry` (Geant4 11.4):** для `INCLXX_HP` правильное имя — `"G4HadronPhysicsINCLXX"` (НЕ `"G4HadronPhysicsINCLXX_HP"`!).
4. **Недостаток `CellPacker`:** при плотной упаковке может молча упаковать меньше `nCells`; фактическое число хранится в `BioDetectorConstruction::GetActualCellCount()`, а масса мишени берётся через `GetScoringMass()`.
5. **Воздушный зазор в `monolayer.mac` без Al-фильтра:** простое комментирование слоя 4 (`1.815 mm G4_Al`) оставляет `1.815 мм` вакуума `G4_Galactic` между электродом Al и слоем воздуха, если не расширить/сдвинуть слой воздуха.
6. **Диапазон `ICSD_Target_*` vs `BioData`:** `ICSD_Target_*` (`0..10000`) обрезает прямой пик `20Ne` в режиме без Al-фильтра (где реальные попадания дают `~45 000–70 000` ионизаций). В `BioData` (`Ions_Total`, `Ions_*`) обрезки нет; при этом `BioData` агрегирует суммы **на событие**, а `ICSD_Target_*` — **на поражённое ядро**.
7. **Одинаковый seed при кластерных запусках:** по умолчанию Geant4 без явного `G4Random::setTheSeed(...)` или `/random/setSeeds` стартует с одним и тем же фиксированным сидом. При параллельном запуске нескольких задач на SLURM/HTCondor это даст идентичные копии данных, и их объединение через `hadd` будет математически некорректным. Нельзя полагаться на `time(nullptr)` (одновременный старт задач в одну секунду даст одинаковый сид) — использовать `std::random_device` (или явный `--seed` от планировщика).
8. **Мастер-`RunAction` создаётся ДО выполнения макроса:** `G4MTRunManager::SetUserInitialization(G4VUserActionInitialization*)` вызывает `BuildForMaster()` немедленно (см. `G4MTRunManager.cc`), то есть мастер-`RunAction` (и схема `BioData`/`H1`) строится в `main.cc` до `/control/execute <macro>`. Следствие: любые параметры, влияющие на схему дерева (скоринг-профиль), **нельзя** задавать UI-командой в макросе — мастер их не увидит, а worker-потоки увидят (`Build()` воркеров вызывается позже), что даёт рассогласование схемы и ошибку `G4RootPNtupleManager::AddNtupleRow ... adding row failed`. Решение: резолвить профиль в `main.cc` (`--profile`, либо предварительное чтение `/bio/score/profile` из макроса) **до** `SetUserInitialization`, тогда мастер и worker видят одинаковое значение.

## 4. Наблюдения по физике на первых данных и актуальные задачи
По первому полному прогону (`monolayer`, без Al-фильтра, `QGSP_BIC_HP`, `3e6` событий, 13 ч на 8 потоках):
- Доза `~0.206 Гр / 10^6` первичных (`EM = 98.35%`, `Hadronic = 1.65%`, где `Heavy Z>=3 = 1.40%`, `Alpha = 0.16%`).
- Второй пик `Heavy (Z>=3)` на `~0.85 МэВ` в `Edep_BySpecies` — прямые пролёты быстрых фрагментов снаряда (`16O/15N` ~29 МэВ/нуклон), левый пик (`~10 кэВ`) — дельта-электроны от пролётов мимо ядра, хвост `2–10 МэВ` — редкие медленные отдачи и ядерные реакции.
- Замечено превышение эффективного ЛПЭ `20Ne` (~`40 MeV/u`) в `DNA_Opt6`: ~`247 кэВ/мкм` против ~`145 кэВ/мкм` по Бете–Блоху.
## 5. Конфиг-слой, скоринг-профили и размещение (Этапы 6+)
- **Стратегия D (единый конфиг):** `config/runs.conf` — источник правды. `scripts/gen_macros.sh` парсит INI-подобные секции и генерирует `config/macros/<scenario>.mac`, `config/macros/manifest.tsv` и `config/plot_scenarios.inc`. Генератор идемпотентен; `config/macros/` и `.inc` править вручную нельзя. 16 сценариев воспроизводят прежние макросы (эквивалентность проверена сравнением нормализованных строк команд).
- **Парсер на bash:** секции `[type:name]`, повторяющиеся ключи (`layer`/`create`/`add`) хранятся в порядке появления (`SEQ[...]`), скаляры — «последний побеждает» (`VAL[...]`). Комментарии — только целой строкой. Валидация: неизвестные ссылки, пустые обязательные поля, нечисловые counts → ошибка с именем секции. При усложнении схемы допустим переход на python.
- **Скоринг-профили:** `include/ScoringProfile.hh` + `src/ScoringProfile.cc` — реестр профилей и **канонический порядок** колонок `BioData` и `H1`. `RunAction` создаёт только каналы активных групп; `EventAction`/`BioSensitiveDetector`/`TrackingAction` заполняют через `ActiveColumnIndex`/`ActiveHistIndex` (`-1`, если группа выключена). Профили: `minimal` (dose-core+ICSD), `standard` (точное историческое дерево 0..34 / H1 0..14), `full` (umbrella).
- **Append-only:** новые каналы — строго в конец канонических списков, иначе сдвигаются индексы `FillNtuple*Column`. `plot.C` читает обязательные core-ветки жёстко, а опциональные группы (species/nonDNA/subchannels/ICSD/спектры) — только при наличии, поэтому `minimal`-файлы анализируются без ошибок.
- **Число событий:** `/bio/run/setEvents <N>` + безголосая `/bio/run/beamOn` (обёртка `G4RunManager::BeamOn`). CLI `--events N` перекрывает макрос (используется кластером). Легаси `/run/beamOn` продолжает работать.
- **Кластер:** `jobs/submit.sh` читает `manifest.tsv`, поддерживает `--dry-run`; `job_run_one.sh` передаёт `--events` (без `sed` макроса). Планировщик — авто-выбор SLURM/HTCondor.
- **`plot.C`:** сценарии — из `config/plot_scenarios.inc`, косметика — из `config/analysis.conf`; отсутствующие `.root` пропускаются, «ни одного сценария» → жёсткая остановка.
- **Размещение в репозитории `geant4`:** `expLRB` — обычная отслеживаемая подпапка (в родительский `.gitignore` добавлено `!expLRB/`); `expLRB/.gitignore` исключает `build/`, `output/`, `*.root`, `logs/`, `*.so`/`*.d`/`*_ACLiC_dict*` и скретч-дампы.

## 8. Кластер HTCondor — грабли (проверено 2026-11)

Раздел про HTCondor-пул группы (адреса/доступ — вне репозитория, в документации не хранятся).
Дополняет §6 (кластер).

- **$HOME на вычислительных узлах read-only.** Задача (процесс на узле) не может писать в общий
  репозиторий: попытки `tee logs/...` и создания `output/raw/<...>.root` дают
  `Permission denied`/`Analysis_W001 Cannot create file`. Поэтому:
  - чанки и любые выходы пишем в **песочницу** узла;
  - логи и чанк возвращаем **через HTCondor**.
- **HTCondor: FS-домены узлов разные** → режим «общего ФС» (`should_transfer_files = NO`) не матчится
  (`TARGET.FileSystemDomain == MY.FileSystemDomain` → 0). Используем transfer-режим:
  `should_transfer_files = YES`, `transfer_executable = False` (скрипт запускается по абсолютному
  пути — репозиторий виден на чтение), `getenv = True` (проброс Geant4-окружения).
- **Возврат чанка:** HTCondor кладёт `transfer_output_files` **по basename** (теряет подкаталоги).
  Рабочая схема (как в chem6): писать чанк в **корень песочницы** (`<scenario>_<proc>.root`), а
  `transfer_output_remaps = "<name> = /abs/repo/output/raw/<name>"` (абсолютное назначение).
  Тогда файл оказывается ровно в `output/raw/`.
- **CRLF ломает shebang.** Если на удалённом клоне включён `core.autocrlf`, `git pull` превращает
  `job_run_one.sh` в CRLF → shebang `#!/usr/bin/env bash\r` → ядро не находит интерпретатор и HTCondor
  даёт `Failed to execute ... errno=2 No such file or directory`. Меры: `expLRB/.gitattributes`
  (`*.sh/*.py/*.in/*.mac/*.conf text eol=lf`), на кластере `git config core.autocrlf false`,
  при необходимости `sed -i 's/\r$//' jobs/*.sh scripts/*.sh`.
- **Geant4-DNA `DNA_Opt6` (CPA100) падает на кастомной воде.** На «inter» штатные сборки Geant4
  11.4 (ref01/ref02/ref08/ref09) падают (SIGSEGV/abort) в `G4DNACPA100Elastic/Excitation/Ionisation
  Model` при работе с не-`G4_WATER` материалами (наш `G4_WATER_LAYER` = `BuildMaterialWithNewDensity`).
  Это известный баг: «CPA100 models only load data for material index 0». Фикс — брать материал через
  `GetBaseMaterial()` в DNA-моделях. На рабочей сборке (WSL2 ref01) это уже пропатчено; на кластере
  нужно скопировать 5 файлов `source/processes/electromagnetic/dna/{models/src/G4DNACPA100{Elastic,
  Excitation,Ionisation}Model.cc,models/src/G4DNAELSEPAElasticModel.cc,utils/src/G4DNAMolecularMaterial.cc}`
  и пересобрать Geant4 (`~/build_geant4.sh`, инкрементально из `/tmp/geant4_build_<pid>`).
- **`G4FORCENUMBEROFTHREADS`** перекрывает `/run/numberOfThreads` из макроса. `job_run_one.sh` ставит
  его из `THREADS` (кластерный дефолт 1), `main.cc` — из `--threads`. Иначе макрос (10 потоков) уводит
  задачу в переподписку CPU.
- **`printProgress` динамический:** `gen_macros.sh` считает `max(10, ceil(events/progress-lines))`
  (по умолчанию `progress-lines=500`), чтобы число строк прогресса было ограничено; перекрывается
  явными `[run:…] print-progress`/`progress-lines`.
- **Мониторинг:** `jobs/monitor.py` (per-job: JOB ID/STAT/PROGRESS/EVENTS/ELAPSED/ETA/SPEED, сводка
  батча). Живой прогресс — через `condor_tail` (т.к. `.out` при read-only home появляется только
  после завершения); общее число событий — из атрибута `+ExpLRBEvents` (иначе `???`). `jobs/monitor.sh`
  — сводка по сценариям (логи/очередь). Refresh: `--refresh`/`--interval`.
- **Тест `test_dedx` ускорен:** для dE/dx-валидации мир уменьшен (`[geometry:dedx] world-xy=20 mm,
  world-z=1 mm`) — на депозит в слое не влияет (`mean Edep_Total ≈ 2.5 МэВ`, `dE/dx ≈ 251 кэВ/µm`),
  но резко сокращает трекинг вторичек в вакууме. Для боевых сценариев мир остаётся 10 см.
- **Память задачи:** задаётся на сценарий в `config/runs.conf` `[sample:…] memory/disk` (МБ) →
  `request_memory/request_disk`/`--mem`. Без `request_memory` HTCondor выдаёт ~128 МБ и задача
  уходит в `H`.
- **`wrk1`/`wrk12` не монтируют `/home`** (проверено 2026-11). Задачи, попавшие на эти узлы, падают с
  `Failed to execute '<repo>/jobs/job_run_one.sh' ... (errno=2: 'No such file or directory')`: узел не
  видит lustre-home, поэтому «запуск executable по абсолютному пути» (`transfer_executable=False`)
  не работает. Остальные узлы (`wrk2..wrk11`) здоровы. Обход без повторной отправки — задать исключение
  на уже поставленных задачах и вернуть их в очередь (завершённые чанки сохраняются):
  ```bash
  condor_qedit -constraint 'true' Requirements '((Machine =!= "wrk1.phys.cluster") && (Machine =!= "wrk12.phys.cluster"))'
  condor_release -all
  ```
  (`condor_qedit` требует `-constraint <expr>` или `<cluster>[.<proc>]`; форма `-all` в 24.3.0 не работает.)
  Лечение на стороне кластера — починить mount на `wrk1`/`wrk12`.

## 9. РАЗРЕШЕНО: расхождение dE/dx DNA на кластере (2026-11)

**Итог:** физически верное значение — чистая EM-остановка **≈146.5 кэВ/µm (≈1.467 МэВ за 10 µm)**.
WSL2 `geant4-v11.4-ref01` (**2.51 МэВ**, 245 кэВ/µm) **физически невозможна** — превышает безусловный
максимум (unrestricted electronic stopping), сборка «молча неверная». Кластер `geant4-11.4-ref09`
(**1.09 МэВ**, 109 кэВ/µm) — в пределах допустимого. Исходная посылка §9 («кластер ≈ EM → DNA не даёт
вклада») была **артефактом**: кластерный EM-прогон был всего на 2 события; на 20 событиях кластерный
EM (1.348) заметно отличается от кластерного DNA (1.093).

**Независимая оценка чистого EM** (штатная утилита `/expLRB/dedx/print` на базе `G4EmCalculator`,
EM_Opt4, без DNA и адроники; `config/dedx_emcalc.mac`), ион при 40 МэВ/нуклон в `G4_WATER`:

| ион (40 МэВ/нуклон) | electronic dE/dx | dE/dx / p(40 МэВ) |
|---|---|---|
| p 40 МэВ | 1.479 кэВ/µm | 1 |
| α 160 МэВ | 5.888 кэВ/µm | 3.98 (=z²) |
| C12 480 МэВ | 52.92 кэВ/µm | 35.8 (≈z²=36) |
| **Ne20 800 МэВ** | **146.53 кэВ/µm** | **99.1 (≈z²=100)** |

Все ∝ z² (полностью ободранные ионы) и согласуются с Бете/ICRU (p(40 МэВ)≈1.4–1.5 по PSTAR). Интеграл
по 10 µm для Ne = **1.467 МэВ** — абсолютный максимум депозита в слое (нарушить нельзя).

**Прогоны `test_dedx`/`test_dedx_em` (20 событий, `--seed 12345 --threads 1`, `Edep_Total`):**

| сборка | EM (`test_dedx_em`, EM_Opt4) | DNA (`test_dedx`, DNA_Opt6) |
|---|---|---|
| WSL2 `geant4-v11.4-ref01` | 1.34812 МэВ | **2.510 МэВ** → НЕВОЗМОЖНО (>1.467) |
| кластер `geant4-11.4-ref09` | 1.34812 МэВ | **1.093 МэВ** → допустимо |

EM-прогоны **побитово идентичны** на обеих сборках (event-by-event) → вся разница в DNA-транспорте иона.

**Причина:** сборки различаются кодом DNA-активатора (`G4EmDNAPhysicsActivator`/`G4EmDNABuilder`).
Для `GenericIon` в BioRegion при заданном масштабе энергии `esc = E·m_p/m_ion` (Ne 800 МэВ → 40.3 МэВ):
- `ref01`: `ionIoni` BetheBloch от **2 МэВ** И `GenericIon_G4DNAIonisation` (Rudd-extended,
  `G4DNAGeneralIonIonisationModel`) до **300 МэВ** → перекрывающееся покрытие → завышение депозита;
- `ref09`: BetheBloch от **100 МэВ**, DNA-Rudd до **100 МэВ** → неперекрывающиеся диапазоны → физичный результат.
Т.е. обработку тяжёлых ионов в DNA **починили между ref01 и ref09**. Ручной CPA100-патч ref01 (§8)
к этому отношения не имеет (он только про segfault на кастомной воде).

**Fix / рекомендации:**
1. Для тяжёлых ионов использовать **ref09** (эталон — EM ≈146.5 кэВ/µm). **Не** копировать DNA-исходники
   ref01 в ref09 — это импортирует неверное поведение.
2. CPA100-патч (если нужен для кастомной воды, §8) применять к **собственным** исходникам ref09 и
   перепроверять тестом + утилитой ниже. Отдельная сборка не требуется.
3. Регрессионный гейт: DNA-депозит иона не должен превышать интеграл EM по толщине
   (`config/dedx_emcalc.mac`); при необходимости сравнить `.root` с эталоном 146.5 кэВ/µm.
4. Физическое решение по DNA для тяжёлых ионов (DNA-модели валидированы для e⁻/γ/p/α и лёгких ионов,
   не для 40 МэВ/нуклон Ne) — отдельно, требует согласования (см. §4 AGENTS).

**Утилита / точки кода:**
`/expLRB/dedx/print <material> <Z> <A> <energy> <unit> [thickness <unit>] [region <name>]`
(`include/EnergyLossMessenger.hh`, `src/EnergyLossMessenger.cc`; доступна после `/run/initialize`,
только master-поток). Запуск: `./build/expLRB -m config/dedx_emcalc.mac --threads 1`.
Прочие точки: `config/runs.conf` `[geometry:dedx]`; `jobs/{htcondor.sub.in,submit.sh,job_run_one.sh,monitor.py}`.

**Доступ к кластеру (для агента):** `ssh -o BatchMode=yes <user>@<host>` по ключу (адрес/учётка — вне
репозитория, не хранить). Репозиторий на кластере: `~/projects/geant4/expLRB` (remote `github`).

---

## 11. Производственная кампания и графики (2026-11-27) — АКТУАЛЬНОЕ СОСТОЯНИЕ

**Кампания завершена** (HTCondor + SLURM, общий lustre-`/home`):
- 16 боевых сценариев: **monolayer — 8 × 20·10⁶** событий (4·10⁶ первично + 16·10⁶ доборный батч),
  **suspension — 8 × 1·10⁶**. Смёржены в `output/root/<scenario>.root`; `output/raw/` — 6720 чанков
  (проверено: monolayer `entries=20e6`, suspension `1e6`). Старые 1e6-данные — в `output/{raw,root}_prev_1e6/`.

**Пайплайн / инструменты:**
- `jobs/submit.sh` — число событий берётся **из `manifest.tsv` (per-scenario)**; `EVENTS_TOTAL` — override
  всех; `RUN_LABEL` (по умолчанию timestamp) в имени чанка → повторные прогоны **накапливают** статистику;
  `SCHEDULER=condor|slurm`; `THREADS=10`.
- `jobs/job_run_one.sh` — **атомарная запись чанка**: пишет во временный файл и `mv` по успеху. Временный
  файл **обязан оканчиваться на `.root`** — иначе `G4AnalysisManager` (формат по расширению) не создаёт файл
  и HTCondor уходит в hold «transfer output failure» (мой прошлый баг, `cc0b8b6`).
- `scripts/merge_all.sh` — пропускает обрубки/недописанные (<10 КБ). `plot.C` — графики.
- `jobs/monitor_all.py` — сводный монитор HTCondor+SLURM+полные чанки (addresses через env `CONDOR_SSH`/
  `SLURM_SSH`, вне репозитория).

**SLURM:** доступен через отдельный логин-хост (ОС Ubuntu, адрес/порт — у админа, вне репозитория);
**общий `/home`** с HTCondor → репозиторий и Geant4 видны. ~39 узлов / ~1836 CPU. `submit.sh` запускать
там, где есть `sbatch`; `JOB_RUNNER` — абсолютный путь к `job_run_one.sh` (SLURM копирует скрипт в spool).

**HTCondor:** ~9 живых узлов по 48 CPU; **wrk1/wrk12 не монтируют `/home`** → исключены в `htcondor.sub.in`
(`requirements`). Задачи просят 10 CPU → 4 задачи/узел (8 CPU «в отход»), поэтому параллелизм ~36.

**Фильтрованные сценарии (`*_filt_*`) — ВАЖНО:** фильтр стоит **специально**, чтобы **остановить пучок**
(20Ne 1 ГэВ = 50 МэВ/нуклон) в последнем фильтре; сигнал — **долетевшие до мишени вторичные фрагменты**.
Поэтому `Edep_PrimaryIonDirect ≈ 0` и малый полный депозит в `*_filt_*` — **ОЖИДАЕМО, не баг**.

**Фрагменты:** append-only векторные ветки `BioData` — `Frag_Z/Frag_A/Frag_Edep/Frag_EdepDelta/Frag_Ions`
(группа скоринга `fragments`, включена в `standard`/`full`); `BioTrackInformation` несёт (Z,A), δ-электроны
наследуют (Z,A) родителя.

**plot.C — текущее состояние (коммиты `dee0ba2`..`b92341d`):**
- `Edep_ByFragment_<scen>` — по элементу Z; **столбец = объединение изотопов**, изотопы — **оттенками
  цвета элемента** (тёмный низ → светлый верх), подпись фрагмента снизу; δ = серая «шапка» сверху.
  Подписи изотопов — **одна горизонтальная строка `A/проц%`** (шрифт авто-подгоняется под ширину
  столбца; тонкие полосы — вертикальный сдвиг + тонкий лидер). `A` — массовое число, `%` — доля
  изотопа в собственном (direct) вкладе элемента (сумма по столбцу = 100%).
- `Edep_BySpecies_<scen>`, `Edep_BySpecies_DirectDelta_<scen>` — легенды **вынесены за фрейм** (правое
  поле); цвета — генератор `DistinctColor` (золотое сечение по тону + циклическая светлота).
- `HadronVertexZ_<scen>` + `Compare_HadronVertexZ` — корректные оси `z [mm]`.
- `Compare_*` — одиночный пад, компактные метки, уникальные цвета по индексу.
- Символы элементов — **полная периодическая таблица** (`ElemSymbol`), Z>20 больше не «?».
- Вывод — **PDF** (`image-format = pdf`), 123 файла в `output/plots/analysis/`; SVG/PNG не пишем.
  Ревизия: `gs -q -dNOPAUSE -dBATCH -sDEVICE=png16m -r150 -sOutputFile=x.png x.pdf`; **ImageMagick SVG
  рендерит плохо** (теряет повёрнутые подписи, ломает верхние индексы) — не использовать.
- Исправлены прежние дефекты: hatched δ (в SVG невидим), 3-цветные SVG-столбики `BAR1`,
  легенды поверх данных, «?»-символы, совпадающие цвета, вырожденные однокомпонентные графики.

---

## 12. Сравнение адронных моделей — ПЛАН и ШАГ 1 (2026-11-28)

**Цель:** сравнить адронные физ. конфигурации на **той же** геометрии/EM, чтобы оценить влияние выбора
адронной модели на фрагменты, входные спектры, ICSD и дозу.

**Решения (интервью, 2026-11-28):**
- Добавить пресеты: `QGSP_BIC`, `QGSP_BERT_HP`, `FTF_BIC`, `Shielding` (к существующим
  `QGSP_BIC_HP`/`FTFP_BERT_HP`/`QBBC`/`INCLXX_HP`). **`QGSP_BIC_AllHP` исключён**: требует
  `G4PARTICLEHPDATA`/`G4PROTONHPDATA`, которых в сборке нет → падает с `G4HadronicException`.
- Механизм задания конфигураций: **«ось конфигураций» + суффикс тега `__<HAD>`** (не дублировать 16
  сценариев-двойников).
- Набор сценариев: **все 16**; статистика — **как в прод** (monolayer `4·10⁶`, suspension `1·10⁶`).
- Именование выходных файлов: суффикс `__<HAD>` для raw/merged/plots.
- Сводные графики: **сетка «величины × конфиги»** на одном холсте (по каждому сценарию), чтобы видеть
  качественную разницу, а не тысячи линий на одной оси.
- Запуск: **новая Agent Manager-сессия**, кластер (**HTCondor + SLURM**), мониторинг
  `jobs/monitor.py` / `jobs/monitor_all.py`.

**Состав новых пресетов** (elastic/stopping/ion заданы в `PhysicsList::BuildHadronicConstructors`,
`G4StoppingPhysics` + `G4IonPhysics` едины для всех новых пресетов):

| Пресет | elastic | inelastic |
|---|---|---|
| `QGSP_BIC` | `G4HadronElasticPhysics` | `G4HadronPhysicsQGSP_BIC` |
| `QGSP_BERT_HP` | `G4HadronElasticPhysicsVI` | `G4HadronPhysicsQGSP_BERT_HP` |
| `FTF_BIC` | `G4HadronElasticPhysics` | `G4HadronPhysicsFTF_BIC` |
| `Shielding` | `G4HadronElasticPhysicsHP` | `G4HadronPhysicsShielding` |

Существующие: `QGSP_BIC_HP` (elastic HP), `FTFP_BERT_HP` (HP), `QBBC` (XS), `INCLXX_HP` (HP →
`G4HadronPhysicsINCLXX`).

Ожидаемый масштаб различий: полная доза ≤1–2% (EM≈98%); полная адронная 10–40%; тяжёлые фрагменты
Z≥3 (Fe/Ta-спалляция) ×1.5–3; p/d/t/α — 10–40%; нейтроны — до ×2; ICSD-хвост 10–30%; **первичная
реакция ²⁰Ne одинакова** во всех пресетах (`G4IonPhysics` → `G4BinaryLightIonReaction`).

**Данные сборки:** есть `G4NDL4.7.1` (HP нейтроны, JENDL-HE), `G4INCL1.3`, `G4ABLA3.3`,
`G4PARTICLEXS4.2`; **нет** `G4PARTICLEHPDATA`/`G4PENDL`/`G4TENDL` → `*AllHP` и `*LEND` недоступны.

**Smoke-замеры** (test_dedx, 20 событий, `--threads 2`, локально): `QGSP_BIC_HP`≈149 с,
`QGSP_BIC`≈154 с, `QGSP_BERT_HP`≈157 с, `Shielding`≈163 с (значительная часть — разовая загрузка
HP-данных), `FTF_BIC` — OK. Все `exit=0`, без фатальных.

**СТАТУС шага 1 — DONE** (коммит `3740d1f`): пресеты добавлены в `PhysicsList` + мессенджер; сборка и
smoke-тесты прошли.

---

**ОТКРЫТО (новая сессия, шаги 3–4):**
1. Реализовать «ось конфигураций»: прогон одного сценария под N адронными пресетами, суффикс
   `__<HAD>` в теге и именах (raw/merged/plots). Правки: `gen_macros.sh`/`runs.conf`/`submit.sh`/
   `plot.C`/загрузчик данных.
2. Сводные **сетки** «величины × конфиги» (по сценарию) в `plot.C`.
3. Запустить сравнительную кампанию (16 сценариев × выбранные конфиги) на HTCondor+SLURM; следить
   через `jobs/monitor.py`/`monitor_all.py`.
4. NOTE (пользователь): пока не понимает «отношения» на лог-оси — дать наглядный разбор (на лог-оси
   «толщина» сегмента = множитель, а не линейная разность; δ = верх−низ в линейных единицах).


