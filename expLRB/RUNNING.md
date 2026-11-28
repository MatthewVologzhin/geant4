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
dna-activator = true[scoring:standard]
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

### Адронные конфигурации (`/physics/setHadronic`)
Доступны (PreInit; см. `PhysicsList::BuildHadronicConstructors`):
`QGSP_BIC_HP` (дефолт), `FTFP_BERT_HP`, `QBBC`, `INCLXX_HP`, `QGSP_BIC`, `QGSP_BERT_HP`,
`FTF_BIC`, `Shielding`, а также `none/off/false` (выключает адронику).
**Недоступны** из-за отсутствующих данных: `*AllHP` (нужен `G4PARTICLEHPDATA`), `*LEND`
(нужен `G4PENDL`). Имя конструктора `INCLXX` в реестре — `G4HadronPhysicsINCLXX` (без `_HP`).

### «Ось конфигураций» адронных моделей (`[axis:hadronic]`)
Чтобы сравнить адронные модели **на той же** геометрии/EM/материалах/cut'ах (меняется только
`/physics/setHadronic`), в `config/runs.conf` есть секция:

```ini
[axis:hadronic]
enabled = true
presets = QGSP_BIC_HP QGSP_BIC QGSP_BERT_HP FTF_BIC Shielding FTFP_BERT_HP QBBC INCLXX_HP
```

При `enabled = true` `scripts/gen_macros.sh` для **каждого сценария, у которого профиль физики
имеет `hadronic-enabled = true`**, доп. генерирует по одному макросу/строке manifest на каждый
пресет с суффиксом тега `__<preset-lowercase>`, напр. `suspension_nofilt_cells_had__ftf_bic`:
- чанки → `output/raw/<tag>__<had>_*.root`;
- merged → `output/root/<tag>__<had>.root`;
- графики → `output/plots/analysis/<Type>_<tag>__<had>.pdf`;
- сводная сетка на базу → `output/plots/analysis/Grid_Configs_<base>.pdf`.

`*_nohad`-сценарии **не** дублируются (их физика не зависит от адронной модели). Отдельный
сценарий можно исключить из оси строкой `had-axis = false` (так исключены `test_dedx*`).

Отправка кампании (manifest-строки с непустым `had` = варианты оси):

```bash
jobs/submit.sh --dry-run --axis                 # предпросмотр вариантов оси
jobs/submit.sh --dry-run monolayer_nofilt_cells_had__ftf_bic
SCHEDULER=condor jobs/submit.sh --axis          # отправить все варианты (HTCondor)
SCHEDULER=slurm  jobs/submit.sh --axis          # или SLURM
```

Без аргументов/`--axis` `jobs/submit.sh` по умолчанию берёт только **базовые** (невариантные)
строки — историческое поведение сохранено. Слежение — `jobs/monitor.py`/`jobs/monitor_all.py`
(имена задач содержат суффикс `__<had>`). Детали и статус — `PROJECT_MEMORY.md` §12.

---

## 2. Сгенерировать макросы

```bash
scripts/gen_macros.sh                 # все сценарии
scripts/gen_macros.sh /path/other.conf  # альтернативный конфиг
```

Пишет:
- `config/macros/<scenario>.mac` (и `config/macros/<scenario>__<had>.mac` для вариантов оси — §1);
- `config/macros/manifest.tsv` (`scenario  geometry  sample  physics  scoring  events  threads  macro  memory  disk  base  had`);
- `config/plot_scenarios.inc` (таблица сценариев для `plot.C`; последние два поля строки — `base`, `had`).

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
`config/macros/manifest.tsv`. Число событий на задачу задаётся `--events` (перекрывает макрос);
**общее число событий на сценарий** берётся из `manifest.tsv` (`events`, из `runs.conf`), а
`EVENTS_TOTAL` — это переопределение сразу для всех сценариев.

### Два планировщика: HTCondor и SLURM

Оба кластера используют **общий lustre-`/home`** — репозиторий, `build/expLRB` и установки Geant4
видны на узлах любого из них. HTCondor-хост даёт ~500 свободных ядер; SLURM (отдельный логин-хост,
ОС Ubuntu) — ~1200 свободных CPU (partitions `all`, `student`, `vclass`). Бинарь, собранный на
AlmaLinux-хосте HTCondor, запускается и на Ubuntu-узлах SLURM (проверено).

Чтобы задействовать SLURM: зайдите на его логин-хост и из того же общего репозитория запустите
`SCHEDULER=slurm jobs/submit.sh …` (`submit.sh` должен исполняться там, где есть `sbatch`/`sinfo`).
Удобно разделять нагрузку: быстрые `monolayer` — на HTCondor, тяжёлые `suspension` — на SLURM.

**Доступ к SLURM (вход только с локальной машины).** Логин-хост SLURM — отдельный узел (Ubuntu),
общий lustre-`/home` с HTCondor. Ключевое: он достижим **только с локальной рабочей машины** и
**не с HTCondor-хоста `inter`** (там TCP до порта `30022` уходит в `timeout`, хотя имя `ss-login`
в `/etc/hosts` присутствует).

Хост: `ss-login` (`10.40.10.0`, резерв `10.40.11.0` / `10.40.12.0`), порт `30022`, пользователь
`mvologzhin`, вход по ключу (как на кластер). Добавьте в `~/.ssh/config` **локальной** машины:

```
Host slurm
    HostName 10.40.10.0
    Port 30022
    User mvologzhin
```

Затем с локальной машины:
```bash
ssh slurm                                          # вход на логин-хост SLURM
ssh slurm 'hostname; sinfo -h -o "%P %a %C"'        # хост + свободные CPU по партициям
ssh slurm 'cd ~/projects/geant4/expLRB && SCHEDULER=slurm jobs/submit.sh --axis <базы>'
```
Проверка, что это ТОТ самый узел и он видит общий репозиторий/выходы: `ssh slurm 'ls
~/projects/geant4/expLRB/output/raw | head'` (там же лежат и HTCondor-чанки). У SLURM логин-пода
**нет ROOT** — финальный `plot.C` запускать на `inter` (там `/usr/bin/root`). Мониторинг обоих
планировщиков — `jobs/monitor_all.py` с `CONDOR_SSH="ssh mvologzhin@inter" SLURM_SSH="ssh slurm"`.

Грабли: SLURM копирует `jobs/slurm_array.sh` в свой spool, поэтому `job_run_one.sh` нельзя искать
относительно `BASH_SOURCE` — путь передаётся абсолютным через переменную `JOB_RUNNER` (её экспортирует
`submit.sh`); иначе `No such file or directory`.

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

## 9. Графики: инвентарь и типичные проблемы

Всё делает `plot.C` (ACLiC): `root -l -q 'plot.C+'` → **PDF** в `output/plots/analysis/` (+
`lrb_analysis_results.root`, `lrb_dosimetric_audit.csv`). Формат задаётся `image-format`
(`pdf` по умолчанию; `svg`/`png` поддержаны, но векторный PDF — предпочтителен: корректные
верхние индексы `10^6`, `^{20}Ne` и он хорошо ложится в векторный просмотр/печать).
Сценарии — из `config/plot_scenarios.inc`, косметика — `config/analysis.conf`. Прогоны `test_*`
пропускаются.

**Основные типы графиков**
- `DoseShare_*`, `Dose_BySpecies_*` — доля/величина дозы по семействам.
- `Edep_Distribution_*`, `Edep_BySpecies_*` (лог) — dN/dlnE энерговыделения за событие.
- `Edep_BySpecies_DirectDelta_*` — по элементу: **solid = полная, dashed = вторичная (δ)**; первичная = разность.
- `Edep_BySubChannel_*` — вклад подканалов (первичный ион / его δ / фрагменты / δ фрагментов).
- `Edep_ByFragment_*` — по элементу Z, direct (solid) + δ (серая «шапка» сверху).
- `HadronVertexZ_*`, `Compare_HadronVertexZ` — z рождения адронных вершин (какой фильтр «опасен»).
- `ICSD_*` (только для `cells`-сценариев с целью-ядром), `EntranceSpectra_*`, `Kinematics_AllSpectra_MeVu_*`,
  `FragmentCounts_*`, `Compare_*` (одиночный пад).
- `Grid_Configs_<base>_...` — **сводная сетка «величины × конфигурации»** (ось `[axis:hadronic]`): один
  холст на базовый сценарий, `3×3` панели-величины (Total/Hadronic E_{dep}, Fragment E_{dep} by element,
  ICSD, Entrance spectra, Heavy E/A, Fragment counts, HadronVertexZ, Dose by family), в каждой панели —
  кривые всех адронных пресетов (легенда справа, вне фрейма). Плюс по-конфиговые суффиксные графики
  `<Type>_<base>__<had>.pdf`. Панель без данных в профиле показывается рамкой «no data in profile».

**Типичные проблемы / правила**
1. **Легенды.** Многокомпонентные легенды выносятся **за пределы фрейма** (в правое поле,
   `DrawLegendOutsideRight`), чтобы не перекрывать кривые/столбики. For stacked-сегментов (direct/δ)
   рисуйте сначала накопленную (direct+δ), затем direct сверху — иначе один сегмент перекрывает другой.
2. **Цвета.** Палитра строится генератором `DistinctColor` (золотое сечение по тону) — различима
   для произвольного числа элементов/сценариев; не задавайте цвета вручную списком.
3. **autoRange.** Для фиксированных окон (`plotEdepBySpeciesLinear` и т.п.) при малых Edep данные
   «прилипают» к краю — используйте autoRange/данные-зависимое окно.
4. **Низконаполненные графики.** Графики, сводящиеся к одной компоненте (только первичный ион,
   без адронов/фрагментов), не генерируются — см. early-return'ы в `plot.C`.
5. **Производительность/счёт чанков.** Не-дописанные чанки — обрубки; `merge_all.sh`/`monitor_all.py`
   их игнорируют. Кампания не считается «готовой», пока `DONE < target` по сценарию.
6. **Визуальная ревизия.** Растеризуйте PDF через ghostscript и смотрите PNG под vision-моделью:
   `gs -q -dNOPAUSE -dBATCH -sDEVICE=png16m -r120 -sOutputFile=x.png x.pdf`. Не растеризуйте SVG
   через ImageMagick — его SVG-рендерер теряет повёрнутые подписи, обрезает заголовки осей и ломает
   верхние индексы; PNG — только промежуточный артефакт ревизии, в результат не коммитится.
