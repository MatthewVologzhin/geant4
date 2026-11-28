---
name: macro-and-runs
description: Создание и проверка конфигов config/runs.conf и Geant4 макросов (.mac) для сценариев У-400М (монослой/суспензия, с Al-фильтром/без, смена адронной/EM физики, скоринг-профили, RNG сиды, тестовые прогоны).
---

# Навык: Подготовка конфигов, макросов и серий расчётов

## Источник правды — config/runs.conf
1. **Не правь `config/macros/*.mac` и `config/plot_scenarios.inc` вручную** — они генерируются `scripts/gen_macros.sh` из `config/runs.conf`.
2. **Добавить сценарий:** создай/переиспользуй секции `[beam]`, `[geometry]`, `[materials]`, `[sample]`, `[physics]`, `[scoring]`, `[run]`, затем `[scenario:<name>]` со ссылками (`geometry=`, `materials=`, `sample=`, `physics=`, `scoring=`, `run=`, `beam=`) и при необходимости переопределениями (`output=`, `label=`, `events=`, `override=`, `<section>.<field>=`). Перегенерируй: `scripts/gen_macros.sh`.
3. **Валидация:** неизвестные ссылки/секции, пустые обязательные поля, нечисловые `cells/events/threads` → ошибка с именем секции. Повторный запуск генератора идемпотентен.

## Правила составления сгенерированного `.mac`
1. **Порядок команд:**
   - `/run/numberOfThreads`, `/physics/*`, `/bio/output/fileName`, `/bio/score/profile`, `/bio/score/target`, `/expLRB/det/*`, `/expLRB/material/*`, `/bio/cell/*`, `/bio/layer/*` — **строго до** `/run/initialize`.
   - `/gun/*`, `/expLRB/gun/setRadius`, `/bio/run/setEvents`, `/bio/run/beamOn` — **после** `/run/initialize`.
   - `/control/execute <override.mac>` — перед `/run/initialize`.
2. **Актуальный синтаксис команд проекта:**
   - `/bio/layer/create <material> <number_cell>` (строго 2 аргумента, без `monolayer`/`suspension`!).
   - `/physics/setHadronic <QGSP_BIC_HP|FTFP_BERT_HP|QBBC|INCLXX_HP|QGSP_BIC|QGSP_BERT_HP|FTF_BIC|Shielding|none>`
     (недоступны `*AllHP`/`*LEND` — нет данных `G4PARTICLEHPDATA`/`G4PENDL`; см. `PROJECT_MEMORY.md` §12)
   - `/physics/enableHadronic <true|false>`
   - `/physics/setEM <EM_Opt4|EM_Opt3|EM_Opt0|...>`
   - `/physics/enableDNAActivator <true|false>`
   - `/physics/setLowestElectronEnergy 7.4 eV`
   - `/expLRB/det/setWorldZ 10 cm`, `/expLRB/det/setPreBioCut 10 um`
   - `/expLRB/material/create <name> <density> <unit> <nComponents>` + повторяющиеся `/expLRB/material/add <element> <frac>`
   - `/bio/score/profile <minimal|standard|full>`, `/bio/score/target <nucleus|cytoplasm|membrane|cell|medium>`
   - `/bio/cell/setCut 10 nm`, `/bio/layer/setCut 0.5 um`
   - `/bio/run/setEvents <N>`, `/bio/run/beamOn`
3. **Проверка геометрии слоёв без наложений и вакуумных дыр:**
   - Всегда сверяй интервалы `[Z_center - dZ/2, Z_center + dZ/2]` соседних слоёв.
   - При отключении Al-фильтра (`1.815 mm`) расширяй/сдвигай слой воздуха, чтобы не оставлять `1.815 mm` вакуума `G4_Galactic`.
4. **Безопасность запусков из-под агента:**
   - В агентном терминале разрешены **только** короткие smoke-тесты (`<= 500` клеток, `--events <= 100`).
   - Боевые задания SLURM/HTCondor агент только генерирует и проверяет (`jobs/submit.sh --dry-run`), не отправляя.
5. **Число событий и сиды:**
   - Число событий задаётся `/bio/run/setEvents` из конфига и перекрывается CLI `--events` (планировщик использует именно `--events`).
   - В статических `.mac` **не прописывай** жёсткий `/random/setSeeds` — должен работать автоматический случайный seed из `main.cc` (либо явный `--seed` планировщика для каждой задачи массива).
   - Жёсткий `/random/setSeeds <s1> <s2>` допустим только в отладочном макросе для воспроизведения конкретного бага.

## Кластер HTCondor (особенности пула)
- **`$HOME` на счётных узлах read-only.** Макрос/скрипт не может писать в общий репозиторий. Чанк
  пишется в песочницу узла (корень: `<scenario>_<proc>.root`) и возвращается через
  `transfer_output_files` + `transfer_output_remaps` (абсолютный путь в `output/raw`).
- **Режим HTCondor:** `should_transfer_files=YES`, `transfer_executable=False` (запуск по абсолютному
  пути), `getenv=True`. «Общий ФС»-режим не матчится (у узлов разные `FileSystemDomain`).
- **CRLF запрещён:** `expLRB/.gitattributes` форсирует `eol=lf`; на клоне `git config core.autocrlf false`.
  CRLF в shebang → HTCondor `errno=2 No such file or directory`.
- **Память/диск** задаются на сценарий: `config/runs.conf` `[sample:…] memory/disk` (МБ). Без
  `request_memory` HTCondor даёт ~128 МБ и задача уходит в `H`.
- **Число событий/потоки:** `EVENTS_TOTAL`/`EVENTS_PER_JOB`/`THREADS` (env). Потоки принудительно —
  `G4FORCENUMBEROFTHREADS` (перекрывает `/run/numberOfThreads` из макроса).
- **`printProgress`** динамический (`gen_macros.sh`): `max(10, ceil(events/progress-lines))`.
- **Монитор:** `jobs/monitor.py` (per-job, `condor_tail`), `jobs/monitor.sh` (по сценариям).
- **DNA_Opt6** требует пропатченной Geant4 (`GetBaseMaterial()` в DNA-моделях) — иначе падение на
  кастомной воде (см. `PROJECT_MEMORY.md` §8).
