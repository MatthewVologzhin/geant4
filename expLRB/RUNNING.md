# RUNNING.md — практическое руководство expLRB

Как описать задачу в `config/runs.conf`, собрать, запустить локально и на кластере,
сменить версию Geant4 и выбрать скоринг-профиль.

---

## 0. Подготовка

```bash
# выбрать инсталляцию Geant4 (опционально; по умолчанию — из config/geant4.conf)
export GEANT4_PREFIX=/home/mvologzhin/programms/geant4-v11.4-ref01/install

# собрать бинарник build/expLRB
scripts/build.sh
```

`scripts/build.sh` использует timestamp-быстрый путь (пропускает сборку, если ничего не
изменилось) и сам вызывает `cmake` при смене префикса/`CMakeLists.txt`. Новые файлы в
`src/`/`include/` подхватываются (`CONFIGURE_DEPENDS`).

---

## 1. Описать задачу в `config/runs.conf`

Конфиг состоит из именованных секций. Минимальный сценарий:

```ini
[beam:ne20_1gev]
particle = ion
ion = 10 20 10
energy = 1.0 GeV
radius = 7 mm

[geometry:my_tract]
world-z = 10 cm
layer = 14 14 0.004 mm G4_Ta 0 0 0.00020 cm
# ... остальные слои ...

[materials:medium]
create = RPMI_1640 1.0145 g/cm3 10
add = O 0.8653
add = H 0.1098
# ... остальные элементы ...

[sample:my_sample]
material = RPMI_1640
cells = 0                 # 0 = сплошная среда (без /bio/cell/*)
size = 14 14 4 mm
pos = 0 0 2.70370 cm
layer-physics = DNA_Opt6
score-target = medium

[physics:had]
hadronic = QGSP_BIC_HP
hadronic-enabled = true
em = EM_Opt4
dna-activator = true

[scoring:standard]
profile = standard

[run:smoke]
events = 50
threads = 4
print-progress = 10

[scenario:my_run]
geometry = my_tract
materials = medium
sample = my_sample
physics = had
scoring = standard
run = smoke
beam = ne20_1gev
output = output/root/my_run.root   # необязательно (по умолчанию output/root/<scenario>.root)
label = My custom run              # подпись в plot.C
```

Правила:
- заголовок секции: `[type:name]`, где `type ∈ {beam, geometry, materials, sample, physics,
  scoring, run, scenario}`;
- комментарии — только целой строкой (символ `#` в начале строки); внутри строки `#` — часть значения;
- повторяющиеся ключи (`layer`, `create`, `add`) сохраняют порядок;
- сценарий может переопределить поля: `events = 500`, `output = ...`, `override = path.mac`,
  либо `<section>.<field>` (например `sample.cells = 500`);
- `override = <path.mac>` подключается как `/control/execute <path>` перед `/run/initialize` —
  для нестандартных ручных правок поверх сгенерированного макроса.

Валидация: неизвестные ссылки/секции, пустые обязательные поля и нечисловые
`cells/events/threads` приводят к ошибке с именем секции.

---

## 2. Сгенерировать макросы

```bash
scripts/gen_macros.sh                 # все сценарии
scripts/gen_macros.sh /path/other.conf  # альтернативный конфиг
```

Пишет:
- `config/macros/<scenario>.mac`
- `config/macros/manifest.tsv` (`scenario  geometry  sample  physics  scoring  events  threads  macro`)
- `config/plot_scenarios.inc` (таблица сценариев для `plot.C`)

Повторный запуск идемпотентен. Не правьте сгенерированные файлы вручную.

---

## 3. Локальный прогон (только smoke)

> Полномасштабные прогоны (`1e6` событий, `186 000`/`905 000` клеток) длятся > 13 часов —
> их запускает кластер, а не локальная машина/агент.

```bash
# валидационный dE/dx: вода, без клеток — быстрый
./build/expLRB -m config/macros/test_dedx.mac --events 20 --threads 2

# явный seed для воспроизведения
./build/expLRB -m config/macros/test_dedx.mac --events 50 --seed 12345

# сгенерированный сценарий монослоя, но с малым числом событий и клеток
# (правьте копию макроса: /bio/layer/create <mat> 500 и --events 20)
```

Seed печатается в `G4cout` (`=== [expLRB] Engine Seed ... ===`) — сохраняйте его для
воспроизведения.

### Аналитическая оценка dE/dx (`G4EmCalculator`)

Независимая оценка чистой EM-остановки (без DNA и адроники), для сверки с прогонами:

```bash
./build/expLRB -m config/dedx_emcalc.mac --threads 1
# или разово, после /run/initialize:
#   /expLRB/dedx/print <material> <Z> <A> <energy> <unit> [thickness <unit>] [region <name>]
#   /expLRB/dedx/print G4_WATER 10 20 800 MeV 10 um
#   /expLRB/dedx/print G4_WATER 10 20 800 MeV 10 um region BioRegion
```

Печатает electronic/nuclear/total dE/dx (кэВ/µm) и интеграл по толщине (МэВ). Для 20Ne 800 МэВ в
`G4_WATER` эталон: **146.5 кэВ/µm → 1.467 МэВ за 10 µm**. Подробности и разбор сборок — `PROJECT_MEMORY.md` §9.

---

## 4. Выбор скоринг-профиля

```bash
./build/expLRB -m config/macros/test_dedx.mac --profile minimal
```

Профиль обычно задаётся в конфиге (`[scoring:...] profile = ...` → строка
`/bio/score/profile ...` в макросе). CLI `--profile` имеет приоритет. Посмотреть профили:
`/bio/score/list` (или `--profile` с неверным именем — выведет допустимые).

Профили: `minimal` (dose-core + ICSD), `standard` (полное историческое дерево),
`full` (standard + будущие группы). `plot.C` корректно анализирует файлы любого профиля:
отсутствующие опциональные ветки пропускаются, а не вызывают ошибку.

---

## 5. Анализ

```bash
cd /home/mvologzhin/projects/geant4/expLRB
root -l -q 'plot.C+'         # ACLiC
```

`plot.C` читает список сценариев из `config/plot_scenarios.inc` и косметику из
`config/analysis.conf`. Отсутствующие `.root` пропускаются с явным `[ERROR]`; если не
загрузился ни один сценарий — жёсткая остановка.

Правка косметики — в `config/analysis.conf` (`output-dir`, `image-format`, `norm-primaries`,
`norm-label`, …). Правка списка сценариев для анализа — через `config/runs.conf`
(закомментируйте ненужные `[scenario:...]` и перегенерируйте).

---

## 6. Кластер

Кластерные задачи по умолчанию **однопоточные** (`THREADS=1`, `--cpus-per-task=1`): задача = 1 ядро
(лучше упаковка/backfill), меньше памяти на ядро, нет конкуренции потоков. Так как время задачи
растёт ≈ пропорционально потокам, `EVENTS_PER_JOB` подбирайте так, чтобы задача укладывалась в
лимит `#SBATCH --time` (сейчас 24 ч) с запасом. Режим настраивается: `THREADS=N`.

```bash
# предпросмотр (ничего не отправляется)
jobs/submit.sh --dry-run
jobs/submit.sh --dry-run monolayer_nofilt_cells_had

# отправка (авто-выбор планировщика по свободным CPU); по умолчанию 1 поток/задача
EVENTS_TOTAL=1000000 EVENTS_PER_JOB=50000 jobs/submit.sh
SCHEDULER=slurm jobs/submit.sh                       # явно SLURM
THREADS=8 jobs/submit.sh                             # при желании — многопоточно

# объединение готовых чанков (только независимые сиды!)
# merged по умолчанию -> output/root/<scenario>.root (то, что читает plot.C)
scripts/merge_all.sh monolayer_nofilt_cells_had output/root output/raw
```

Планировщик выбирается `jobs/probe_schedulers.sh`; список сценариев — из
`config/macros/manifest.tsv`. Число событий на задачу задаётся `--events` (перекрывает макрос).

### Память и диск на задачу

Ресурсы задаются **на сценарий** в `config/runs.conf`, в секции `[sample:...]`:
```ini
[sample:suspension_cells]
...
memory = 8000      # МБ -> HTCondor request_memory / SLURM --mem
disk   = 4000      # МБ -> HTCondor request_disk
```
`gen_macros.sh` переносит их в `manifest.tsv` (колонки `memory`, `disk`), а `submit.sh` применяет
к каждой задаче. Переопределение на всю отправку — переменными `MEMORY`/`DISK` (МБ):
```bash
MEMORY=16000 jobs/submit.sh monolayer_filt_cells_had
```
Важно: без `request_memory` HTCondor выдаёт крошечный cgroup-лимит (~128 МБ) и задача падает в
`held`. Фактическое потребление смотрите так: `condor_q -af ClusterId ProcId MemoryUsage` (во время),
`condor_history -af ClusterId ProcId MemoryUsage` (после).

### HTCondor: особенности этого пула

Счётные узлы имеют **разные `FileSystemDomain`** и **`$HOME` только для чтения**. Поэтому
`jobs/htcondor.sub.in` использует **transfer-режим** (`should_transfer_files = YES`):
- `transfer_executable = False` — скрипт запускается по абсолютному пути (репозиторий виден на чтение);
- `getenv = True` — проброс Geant4-окружения;
- чанк пишется в **песочницу** (корень: `<scenario>_<proc>.root`), затем возвращается домой
  `transfer_output_files` + `transfer_output_remaps` (абсолютный путь в `output/raw/`) — иначе
  HTCondor кладёт файл по basename вне `output/raw`.
- Перед отправкой подгрузите окружение: `source config/geant4.conf && source "$GEANT4_PREFIX/bin/geant4.sh"`.

Типичные проблемы:
- `H` (`condor_q -held`): чаще память → поднять `MEMORY=`/`[sample:…] memory`; либо
  `Failed to execute … errno=2 No such file or directory` → **CRLF в shebang** (см. ниже).
- **CRLF:** при `core.autocrlf` на удалённом клоне `git pull` превращает скрипты в CRLF и ломает
  shebang. В репозитории есть `expLRB/.gitattributes` (`eol=lf`); на кластере:
  `git config core.autocrlf false` и, при необходимости, `sed -i 's/\r$//' jobs/*.sh scripts/*.sh`.
- **DNA-падение (CPA100):** если Geant4-сборка не пропатчена (см. `PROJECT_MEMORY.md` §8), `DNA_Opt6`
  падает на кастомной воде; нужна сборка с `GetBaseMaterial()` в DNA-моделях.

### Мониторы прогресса и ETA

Есть два монитора:

**`jobs/monitor.py`** (рекомендуется для HTCondor) — «жизненный цикл» по каждой задаче:
`JOB ID | STAT | PROGRESS | EVENTS | ELAPSED | ETA | SPEED` + сводка сверху (RUN/IDLE/DONE/HELD,
общий прогресс, суммарная скорость, ETA всего батча). Живой прогресс читается через `condor_tail`
(на вычислительных узлах `$HOME` read-only, поэтому `.out` появляется в репозитории только по
завершении задачи); по завершении используется локальный `logs/…out`.
```bash
jobs/monitor.py                    # интерактивный дашборд (Ctrl-C / q — выход)
jobs/monitor.py --static           # один снимок в stdout (удобно «вписать строку»)
jobs/monitor.py --refresh 5        # период обновления, сек (по умолчанию 3)
jobs/monitor.py --scenario test_dedx
```

**`jobs/monitor.sh`** — сводка **по сценариям** (done/run/pend, события, rate, ETA), читает логи и
очередь; удобен для SLURM/локально:
```bash
EVENTS_TOTAL=1000000 EVENTS_PER_JOB=50000 THREADS=1 jobs/monitor.sh --interval 30
```

Замечание о «шаге» прогресса: счётчик растёт по мере печати строк `--> Event N` в логе.
Частота печати (`/run/printProgress`) вычисляется **динамически** генератором из размера прогона:
`print-progress = max(10, ceil(events / progress-lines))`, где `progress-lines` (по умолчанию 500) —
желаемое число строк. Так для `1e6` событий шаг = 2000 (≈500 строк), для малых прогонов — не чаще,
чем раз в 10 событий. Переопределяется явными `[run:…] print-progress = N` или `progress-lines = M`.

```bash
# передавайте те же EVENTS_*, что и в submit (иначе число задач в оценке будет другим)
THREADS=1 EVENTS_TOTAL=1000000 EVENTS_PER_JOB=50000 jobs/monitor.sh    # живой watch (обновление 60 c)
jobs/monitor.sh --once                                                 # один снимок
jobs/monitor.sh --interval 30 --tasks monolayer_nofilt_cells_had       # детализация по задачам
```

Прогресс читается из `logs/expLRB_<scenario>_<jobid>_<idx>.out` (такие имена задаёт `jobs/submit.sh`).

---

## 7. Смена версии Geant4

```bash
export GEANT4_PREFIX=/path/to/other/geant4/install
scripts/build.sh               # переконфигурирует CMake, пересоберёт
```

Либо отредактируйте `GEANT4_PREFIX`/`GEANT4_VERSION_NAME` в `config/geant4.conf`.

---

## 8. Рецепт: добавить новый канал/группу (аддитивно)

1. Заведите группу в `enum class ScoreGroup` (`include/ScoringProfile.hh`).
2. Добавьте новые **колонки в конец** `CanonicalNtupleColumns()` (в `src/ScoringProfile.cc`)
   и/или гистограммы в конец `CanonicalHistograms()`. Старые индексы не трогайте!
3. Включите группу в нужные профили (как минимум в `full`).
4. Создайте гистограмму в `RunAction::CreateHistogram` (если это H1).
5. Заполняйте канал в `EventAction`/`BioSensitiveDetector`/`TrackingAction` через
   `ActiveColumnIndex(...)` / `ActiveHistIndex(...)` (вернёт `-1`, если группа выключена).
6. При необходимости дополните `plot.C` (читайте новые ветки опционально).
7. `scripts/build.sh`; проверьте, что `standard` даёт прежнее дерево, а `full` — с новым.

Профиль `standard` обязан сохранять колонки 0..34 (`plot.C` и старые данные на них опираются).
