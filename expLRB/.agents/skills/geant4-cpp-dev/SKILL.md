---
name: geant4-cpp-dev
description: Разработка, отладка, настройка физики/скоринга и сборка C++ классов Geant4 11.4 в проекте expLRB. Использовать при любых правках в src/, include/, main.cc или CMakeLists.txt.
---

# Навык: Geant4 11.4 C++ разработка и отладка

## Порядок действий при правке C++
1. **Сверка API до написания кода:** при использовании любых классов `G4*` проверь точные заголовки и сигнатуры в `/home/mvologzhin/programms/geant4-v11.4-ref01/install/include/Geant4/` (через `grep` / чтение `.hh`). Исходники Geant4 (для понимания поведения) — в `/home/mvologzhin/programms/geant4-v11.4-ref01/geant4/source/`.
2. **Проверка жизненного цикла MT:**
   - `ConstructParticle()` вызывается в `SetUserInitialization`, до макросов. Настройка процессов/порогов по флагам из макроса — строго в `ConstructProcess()`.
   - **Мастер-`RunAction` строится НЕМЕДЛЕННО** в `G4MTRunManager::SetUserInitialization` (через `BuildForMaster()`), т.е. до выполнения макроса. Всё, что влияет на схему `BioData`/`H1` (скоринг-профиль), должно быть известно в `main.cc` до создания run manager — см. `ScoringProfile.hh` и `main.cc`.
   - Worker-экземпляры (`BioSensitiveDetector`, `EventAction`, `TrackingAction`, `RunAction` в `Build()`) не должны иметь несинхронизированных общих записей; агрегацию за ран делать через `G4Accumulable` (`accManager->Register(...)`) и `G4AnalysisManager` (`SetNtupleMerging(true)`).
   - Не трогать `include/nanoflann.hpp`.
3. **Скоринг-профили (append-only):** каналы `BioData`/`H1` описаны в `include/ScoringProfile.hh` (`CanonicalNtupleColumns`, `CanonicalHistograms`). Новые колонки/гистограммы добавляй **строго в конец**; создавай в `RunAction`, заполняй через `ActiveColumnIndex`/`ActiveHistIndex` (возвращают `-1`, если группа выключена). Профиль `standard` обязан воспроизводить исторические индексы 0..34.
4. **Синхронизация `.hh` и `.cc`:** при изменении сигнатур конструкторов или полей обновляй оба файла в одном проходе. Новые файлы в `src/`/`include/` подхватываются CMake (`CONFIGURE_DEPENDS`).
5. **Сборка и обязательный smoke-тест:**
   - Собери: `scripts/build.sh` (или `cmake --build build -j`).
   - Быстрый smoke (вода, без клеток): `./build/expLRB -m config/macros/test_dedx.mac --events 20 --threads 2`. Проверь отсутствие `G4Exception`/`Fatal`, печать seed и профиля (`=== [expLRB] Scoring profile: ... ===`), завершение `Run completed`.
   - Для проверки схемы профиля: `--profile minimal|standard|full` и сверь набор объектов в выходном `.root` коротким `root -l -b -q` one-liner.
6. **Очистка неиспользуемого кода:** прежде чем удалять любой файл, проверь `grep -rn` по всему `src/`, `include/`, `main.cc` и `CMakeLists.txt`.
