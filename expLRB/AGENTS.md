# AGENTS.md — expLRB (Geant4-DNA / ЛРБ ОИЯИ)

## 1. О проекте
Приложение на **Geant4 11.4 (`geant4-11-04-ref-01 [MT]`, C++20)** и **ROOT (ACLiC)** для моделирования радиационно-биологических экспериментов на ускорителе У-400М (ЛРБ ОИЯИ): облучение монослоя (`186 000` клеток) и суспензии (`905 000` клеток) пучком ионов `20Ne` (50 МэВ/нуклон, 1.0 ГэВ).
Цель — количественно оценить вклад вторичных ядерных фрагментов (`p, d, t, 3He, alpha, Z>=3, n`), возникающих в фильтрах и стенке пробирки, в поглощённую дозу, распределения кластеров ионизации (**ICSD**) и входные кинетические спектры в клеточных ядрах.

Документация: научная память и «грабли» — **`PROJECT_MEMORY.md`**; практическое руководство (конфиг, запуск, кластер, профили) — **`RUNNING.md`**; обзор — **`README.md`**. Читай `PROJECT_MEMORY.md` при планировании физических/архитектурных изменений.

## 2. Карта репозитория и ключевые файлы
- Корень проекта: `/home/mvologzhin/projects/geant4/expLRB`
- Системные заголовки Geant4 (для проверки API, не угадывать!): `/home/mvologzhin/programms/geant4-v11.4-ref01/install/include/Geant4/`
- Конфиг-слой (источник правды):
  - `config/runs.conf` — сценарии (секции `[beam|geometry|materials|sample|physics|scoring|run|scenario:...]`).
  - `config/analysis.conf` — косметика `plot.C`.
  - `config/geant4.conf` — выбор инсталляции Geant4.
  - `config/macros/` и `config/plot_scenarios.inc` — **ГЕНЕРИРУЮТСЯ** (`scripts/gen_macros.sh`), вручную не править.
- Исходники C++ (`include/` и `src/`):
  - `main.cc` — точка входа, CLI (`--mac/-m --out/-o --threads/-t --seed/-s --events/-n --profile/-p`, позиционные `<macro> [threads]`), генерация seed, batch/GUI автодетект, резолв скоринг-профиля до создания `G4RunManager`.
  - `ScoringProfile.hh/.cc` — реестр скоринг-профилей и **канонический порядок** колонок `BioData`/гистограмм (append-only!).
  - `PhysicsList.*`, `PhysicsListMessenger.*` — модульная физика (`G4DecayPhysics`, базовый EM, `G4EmDNAPhysicsActivator`, переключаемая адроника).
  - `DetectorConstruction.*`, `DetectorMessenger.*` — мир, фильтры (`addLayer`), `PreBioRegion` на последнем пассивном слое.
  - `BioDetectorConstruction.*`, `CellPacker.hh`, `TargetDef.hh` — биослой, 3-слойная клетка (`nucleus`, `cytoplasm`, `membrane`), KD-Tree упаковка, `CellRegion` / `BioRegion`, скоринг; UI-команды `/bio/cell/*`, `/bio/layer/*`, `/bio/score/*`.
  - `MaterialMessenger.*` — создание составных материалов из макроса (`/expLRB/material/*`).
  - `TrackingAction.*`, `BioTrackInformation.hh` — генеалогия треков (`PrimaryEM` vs `HadronicCascade` + `FragmentSpecies`/`SubChannel`).
  - `BioSensitiveDetector.*`, `CellHit.hh` — скоринг `Edep`, дискретных `G4DNAIonisation` и входных спектров `SpecIn_*` (заполнение гейтится профилем).
  - `EventAction.*`, `RunAction.*`, `RunMessenger.*` — заполнение `BioData` (`TTree`), гистограмм `H1`, `G4Accumulable` сводка дозы; `/bio/output/fileName`, `/bio/run/setEvents`, `/bio/run/beamOn`.
  - `PrimaryGeneratorAction.*`, `PrimaryGeneratorMessenger.*` — пучок (`/expLRB/gun/*`).
  - **НЕ ТРОГАТЬ И НЕ РЕФАКТОРИТЬ:** `include/nanoflann.hpp` (сторонний header-only v1.14.0, не тратить на него контекст).
- Скрипты/задания: `scripts/{build.sh,gen_macros.sh,merge_all.sh,merge_runs.sh}`, `jobs/{submit.sh,slurm_array.sh,htcondor.sub.in,job_run_one.sh,probe_schedulers.sh}`.
- Анализ: `plot.C` (сценарии — из `config/plot_scenarios.inc`, косметика — из `config/analysis.conf`), `monolayer.mac`/`test.mac`/`vis.mac` — legacy.

## 3. Сборка и тестовые команды
```bash
# Сборка (подхватывает config/geant4.conf; сам переконфигурирует CMake при необходимости)
scripts/build.sh

# Перегенерация макросов/манифеста/таблицы из config/runs.conf (идемпотентно)
scripts/gen_macros.sh

# Быстрый smoke-тест (вода, без клеток; задавать малое число событий)
./build/expLRB -m config/macros/test_dedx.mac --events 20 --threads 2

# Скоринг-профиль и число событий
./build/expLRB -m config/macros/test_dedx.mac --events 20 --profile minimal
```

Анализ ROOT (через ACLiC, из корня проекта):
```bash
root -l -q 'plot.C+'
```

Сухой прогон отправки на кластер:
```bash
jobs/submit.sh --dry-run
```

## 4. Жёсткие ограничения и правила безопасности
1. **ЗАПРЕТ НА ДОЛГИЕ ЗАПУСКИ:** никогда не запускай `./build/expLRB` на полноразмерных макросах (`events > 1000` или `186000`/`905000` клеток в тесте без отдельного явного разрешения). Полномасштабный прогон длится > 13 часов! Для тестов используй `--events 20..100` и малое число клеток (`500`).
2. **ЗАПРЕТ НА КЛАСТЕР И САМОВОЛЬНОЕ ИЗМЕНЕНИЕ ФИЗИКИ:** не отправляй задания в очереди кластера (`sbatch`, `qsub`, `condor_submit`) и не меняй без согласования параметры геометрии тракта, материалы, модели (`DNA_Opt6`, `QGSP_BIC_HP`), пороги/cut'ы или формулу дозы. `jobs/submit.sh --dry-run` — безопасен.
3. **ЗАЩИТА ОТ ГАЛЛЮЦИНАЦИЙ API:** перед правкой классов Geant4 11.4 или ROOT сверяй точные имена методов, сигнатуры и строковые ключи конструкторов в `/home/mvologzhin/programms/geant4-v11.4-ref01/install/include/Geant4/` (или через `grep`).
4. **ЗАПРЕТ НА ФАЛЬСИФИКАЦИИ В АНАЛИЗЕ:** в скриптах анализа запрещены скрытые fallback'и (подстановка дефолтных веток/файлов при их отсутствии, жёсткая подмена данных или фиктивные линии). При отсутствии нужной **обязательной** ветки/файла — явная ошибка. Опциональные группы (species/nonDNA/subchannels/ICSD/спектры) читаются только если профиль их записал.
5. **APPEND-ONLY ДЕРЕВО:** новые колонки `BioData`/гистограммы добавляются строго в конец канонических списков (`include/ScoringProfile.hh`); профиль `standard` обязан воспроизводить исторические колонки 0..34 и H1 0..14.
6. **СОХРАНЕНИЕ ТОКЕНОВ:** не читай целиком `include/nanoflann.hpp` и сторонние крупные `.root`/`.csv` файлы; для инспекции `.root` используй короткий `root -l -b -q` one-liner.
7. **ОБЯЗАТЕЛЬНЫЙ СЛУЧАЙНЫЙ SEED ГПСЧ:** при каждом запуске бинарника автоматически генерируется независимый положительный seed (`std::random_device` + `std::uniform_int_distribution<long>(1, 2000000000L)`) с вызовом `G4Random::setTheSeed(seed)` **до** `/bio/run/beamOn` и обязательной печатью сида в `G4cout`. Переопределение — через CLI `--seed` (кластерные массивы) или `/random/setSeeds`.
8. **MT-СХЕМА АНАЛИЗА:** `G4MTRunManager::SetUserInitialization` строит мастер-`RunAction` немедленно, поэтому скоринг-профиль резолвится в `main.cc` до создания run manager; мастер- и worker-потоки обязаны иметь идентичную схему дерева.

## 5. Специализированные навыки (`.agents/skills/`)
- Правка C++ кода Geant4 11.4, `PhysicsList`, `SD`, MT, скоринг-профили и пересборка -> `.agents/skills/geant4-cpp-dev/SKILL.md`
- Написание/правка `plot.C`, аудит `.root`-данных, `hadd` -> `.agents/skills/root-analysis/SKILL.md`
- Подготовка/проверка `.mac` и `config/runs.conf` (фильтры, модели, сиды) -> `.agents/skills/macro-and-runs/SKILL.md`
