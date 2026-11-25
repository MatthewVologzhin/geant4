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

```bash
# предпросмотр (ничего не отправляется)
jobs/submit.sh --dry-run
jobs/submit.sh --dry-run monolayer_nofilt_cells_had

# отправка (авто-выбор планировщика по свободным CPU)
EVENTS_TOTAL=1000000 EVENTS_PER_JOB=50000 jobs/submit.sh
SCHEDULER=slurm THREADS=10 jobs/submit.sh

# объединение готовых чанков (только независимые сиды!)
scripts/merge_all.sh monolayer_nofilt_cells_had output/merged output/raw
```

Планировщик выбирается `job/probe_schedulers.sh`; список сценариев — из
`config/macros/manifest.tsv`. Потоки по умолчанию берутся из манифеста (можно переопределить
переменной `THREADS`), число событий на задачу — через `--events` (перекрывает макрос).

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
