# expLRB — Geant4-DNA модель радиобиологического эксперимента (У-400М, ЛРБ ОИЯИ)

Приложение на **Geant4 11.4 (MT, C++20)** с анализом на **ROOT (ACLiC)** для моделирования
облучения биологического образца пучком ионов `20Ne` (`50 МэВ/нуклон`, `1.0 ГэВ`) на
ускорителе У-400М: монослой (`186 000` клеток) и суспензия (`905 000` клеток).

Цель — количественно оценить вклад вторичных ядерных фрагментов (`p, d, t, 3He, α, Z≥3, n`),
рождающихся в фильтрах тракта и стенке пробирки, в:
- поглощённую дозу (EM vs. адронная компонента);
- распределения кластеров ионизации (**ICSD**);
- входные кинетические спектры частиц в клеточном ядре (`SpecIn_*`).

> Это самостоятельный проект, а не пример Geant4. Старое содержимое README (TestEm5)
> было заменено. Научная память и «грабли» — в [`PROJECT_MEMORY.md`](PROJECT_MEMORY.md);
> практическое руководство — в [`RUNNING.md`](RUNNING.md).

---

## 1. Быстрый старт

```bash
# 1. Сборка (подхватывает config/geant4.conf, ставит окружение Geant4)
scripts/build.sh

# 2. Сгенерировать матрицу макросов из конфига
scripts/gen_macros.sh

# 3. Один короткий прогон (валидационный dE/dx, 20 событий)
./build/expLRB -m config/macros/test_dedx.mac --events 20

# 4. Анализ
root -l -q 'plot.C+'      # из корня проекта (ACLiC)
```

Полномасштабный прогон (`/bio/run/setEvents 1000000`) длится > 13 часов — запускайте его
только на кластере (см. [`RUNNING.md`](RUNNING.md)), а не локально.

---

## 2. Конфигурация как источник правды

Описание эксперимента задаётся одним именованным конфигом **`config/runs.conf`**
(INI-подобные секции `[type:name]`). Из него генерируются макросы, манифест запусков и
таблица сценариев для `plot.C`:

```
config/runs.conf
   ├── scripts/gen_macros.sh ──► config/macros/<scenario>.mac   (Geant4 макросы)
   │                          ├─► config/macros/manifest.tsv    (список задач кластера)
   │                          └─► config/plot_scenarios.inc     (сценарии для plot.C)
   └── jobs/submit.sh  читает manifest.tsv и раскидывает события по задачам
```

Типы секций:

| Секция        | Назначение                                                        |
|---------------|-------------------------------------------------------------------|
| `[beam:...]`  | частица, ион, энергия, радиус пучка                              |
| `[geometry:...]` | слои тракта (`layer = ...`) и параметры мира (`world-z`, ...)  |
| `[materials:...]` | составные материалы (`create = ...` + повторяющиеся `add = ...`) |
| `[sample:...]` | биослой: материал, число клеток, размер/позиция, геометрия клетки, физика слоя, скоринг-мишень |
| `[physics:...]` | адронная модель, EM-опция, DNA-активатор                       |
| `[scoring:...]` | скоринг-профиль (см. §4)                                         |
| `[run:...]`   | число событий, потоков, прогресс                                 |
| `[scenario:...]` | сценарий: ссылки на профили (`geometry=`, `sample=`, `physics=`, `scoring=`, `run=`, `beam=`, `materials=`) + переопределения (`output=`, `override=`, `label=`, `color=`, `events=` …) |

Комментарии — только целой строкой (`#` в начале строки). Повторяющиеся ключи
(`layer`, `create`, `add`) сохраняют порядок. Повторный запуск генератора идемпотентен;
битый конфиг завершается понятной ошибкой с именем секции.

Пример добавления сценария — в [`RUNNING.md`](RUNNING.md).

---

## 3. Дерево результатов (`BioData`)

`RunAction` создаёт `TTree BioData` и гистограммы `H1`. Набор каналов определяется
активным **скоринг-профилем** (§4). Профиль `standard` воспроизводит исторический набор:

- **Колонки `BioData` (0..34):** `EventID`, `Edep_Total/EM/HadTotal`, `Edep_{Proton…Other}`,
  `Ions_Total/EM/HadTotal`, `Ions_{Proton…Other}`, `HitTargetCount`, `Ions_NonDNA*`,
  `Edep_{PrimaryIonDirect,PrimaryDeltaRays,HadronicFragmentsDirect,HadronicDeltaRays}`,
  `Ions_{…}` для тех же подканалов.
- **Гистограммы `H1` (0..14):** `ICSD_Event_Total`, `ICSD_Target_{EM,Had,Total}`,
  `SpecIn_AllHad/Proton/Deuteron/Triton/He3/Alpha/Heavy/Neutron/HeavyPerNucl`,
  `SpecIn_Electron`, `HadronVertexZ`.

**Дисциплина «append-only»:** новые каналы добавляются **строго в конец** канонических
списков в `include/ScoringProfile.hh`, иначе меняются индексы `FillNtuple*Column`.

---

## 4. Скоринг-профили

Профиль — именованный набор групп вывода (`include/ScoringProfile.hh`):

| Профиль    | Группы                                                              |
|------------|---------------------------------------------------------------------|
| `minimal`  | dose-core, ICSD                                                     |
| `standard` | все текущие группы (точное историческое дерево, колонки 0..34)      |
| `full`     | umbrella: `standard` + будущие аддитивные группы                    |

Группы: `dose-core`, `dose-by-species`, `nonDNA`, `subchannels`, `spectra-by-species`,
`electron-spectrum`, `ICSD`, `hadron-vertex-z`. Выбор профиля:

- в конфиге: `[scoring:name] profile = standard` → макрос `/bio/score/profile standard`;
- CLI: `./build/expLRB -m m.mac --profile minimal` (перекрывает макрос);
- интерактивно: `/bio/score/list`, `/bio/score/profile <name>`.

> **Важно:** профиль резолвится в `main.cc` **до** создания `G4RunManager`
> (`G4MTRunManager::SetUserInitialization` строит мастер-`RunAction` немедленно). Поэтому
> `--profile` и предварительное чтение `/bio/score/profile` из макроса обязательны, чтобы
> схема дерева совпадала у мастер- и worker-потоков.

---

## 5. Число событий и запуск

- `/bio/run/setEvents <N>` планирует число событий, безголосая `/bio/run/beamOn` запускает
  `G4RunManager::BeamOn(N)`.
- CLI `--events N` перекрывает значение из макроса (используется планировщиком).
- Легаси-макросы с `/run/beamOn` продолжают работать (встроенная команда Geant4).

CLI бинарника `build/expLRB`:

```
-m, --mac <file.mac>      макрос (batch)
-o, --out <file.root>     выходной ROOT (перекрывает /bio/output/fileName)
-t, --threads <N>         число потоков
-s, --seed  <N>           явный seed ГПСЧ (иначе — случайный, печатается)
-n, --events <N>          число событий (перекрывает /bio/run/setEvents)
-p, --profile <name>      скоринг-профиль (перекрывает /bio/score/profile)
<macro> [threads]         позиционная совместимость
```

### UI-команды (кратко)

Физика (PreInit): `/physics/setHadronic`, `/physics/enableHadronic`, `/physics/setEM`,
`/physics/enableDNAActivator`, `/physics/setLowestElectronEnergy`.
Детектор: `/expLRB/det/setWorldMat|setWorldXY|setWorldZ|setPreBioCut`, `/expLRB/det/addLayer`.
Материалы: `/expLRB/material/create`, `/expLRB/material/add`.
Биообразец: `/bio/cell/{geometry,materials,densities,physics,setCut}`,
`/bio/layer/{create,size,pos,physics,setCut}`.
Скоринг: `/bio/score/{target,profile,list}`.
Вывод и запуск: `/bio/output/fileName`, `/bio/run/setEvents`, `/bio/run/beamOn`.
Пучок: `/gun/particle`, `/gun/ion`, `/gun/energy`, `/expLRB/gun/setRadius`.

---

## 6. Кластер

```bash
# Проверить план без отправки
jobs/submit.sh --dry-run
jobs/submit.sh --dry-run monolayer_nofilt_cells_had

# Отправить (авто-выбор SLURM/HTCondor по свободным CPU)
EVENTS_PER_JOB=50000 jobs/submit.sh

# После завершения — объединить чанки (только независимые сиды!)
scripts/merge_all.sh <scenario> output/merged output/raw
```

`jobs/submit.sh` берёт список сценариев из `config/macros/manifest.tsv`; каждый чанк
запускается с собственным seed через `--seed` и собственным числом событий через `--events`.

---

## 7. Структура проекта

```
expLRB/
├── main.cc                     # точка входа: CLI, seed, batch/UI, профиль
├── include/ src/               # C++ (Geant4); nanoflann.hpp — сторонний, не трогать
│   ├── ScoringProfile.hh/.cc   # реестр скоринг-профилей и канонических каналов
│   ├── Detector*/BioDetector*  # геометрия тракта и биообразца
│   ├── PhysicsList*            # модульная физика (EM/DNA/адроника)
│   ├── *Action*, *Messenger*   # скоринг, вывод, UI
├── config/
│   ├── runs.conf               # ИСТОЧНИК ПРАВДЫ (сценарии)
│   ├── analysis.conf           # косметика plot.C
│   ├── geant4.conf             # выбор инсталляции Geant4
│   ├── macros/                 # ГЕНЕРИРУЕТСЯ: *.mac + manifest.tsv
│   └── plot_scenarios.inc      # ГЕНЕРИРУЕТСЯ: сценарии для plot.C
├── scripts/
│   ├── build.sh                # сборка с выбором Geant4
│   ├── gen_macros.sh           # config/runs.conf → макросы + манифест + .inc
│   └── merge_all.sh merge_runs.sh
├── jobs/
│   ├── submit.sh               # отправка по манифесту (+ --dry-run)
│   ├── slurm_array.sh htcondor.sub.in job_run_one.sh probe_schedulers.sh
├── plot.C                      # ROOT-анализ и аудит (ACLiC)
├── RUNNING.md PROJECT_MEMORY.md AGENTS.md
└── (legacy) monolayer.mac test.mac vis.mac
```

**Не редактируйте** файлы в `config/macros/`, `config/plot_scenarios.inc` — правьте
`config/runs.conf` и перегенерируйте. Легаси-макросы (`monolayer.mac`, `test.mac`, `vis.mac`)
оставлены для совместимости; `test.mac` использует устаревший 3-аргументный
`/bio/layer/create` и не запустится без правки.

---

## 8. Сборка и требования

- Geant4 **11.4** (`geant4-11-04-ref-01 [MT]`), C++20; ROOT для анализа (ACLiC, `hadd`).
- Инсталляция Geant4 задаётся в `config/geant4.conf` (`GEANT4_PREFIX`) или через переменную
  окружения; `scripts/build.sh` сам подхватывает `${prefix}/bin/geant4.sh` и
  переконфигурирует CMake при смене префикса.
- `WITH_GEANT4_UIVIS=OFF` собирает бинарник без UI/визуализации (batch-only).
