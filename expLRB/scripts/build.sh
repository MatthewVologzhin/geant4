#!/usr/bin/env bash
# Fast incremental build/rebuild for expLRB with a selectable Geant4 version.
#
# - Sources config/geant4.conf (edit it, or export GEANT4_PREFIX, to switch Geant4).
# - Skips compiling if no source/header/CMakeLists file is newer than the binary
#   (timestamp check over src/, include/, main.cc, CMakeLists.txt).
# - Reconfigures automatically when the Geant4 prefix or CMakeLists.txt changed.
#
# Usage:
#   scripts/build.sh                 # Release (default)
#   BUILD_TYPE=Debug scripts/build.sh
#   GEANT4_PREFIX=/opt/geant4/11.3 scripts/build.sh
#   BUILD_DIR=build-hpc scripts/build.sh
#   clean=1 scripts/build.sh         # wipe the build dir first
set -euo pipefail

SELF_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SELF_DIR/.." && pwd)"

# --- Geant4 selection ---
# shellcheck source=/dev/null
[[ -f "$ROOT/config/geant4.conf" ]] && source "$ROOT/config/geant4.conf"
GEANT4_PREFIX="${GEANT4_PREFIX:-/opt/geant4}"
if [[ -f "$GEANT4_PREFIX/bin/geant4.sh" ]]; then
  # shellcheck source=/dev/null
  source "$GEANT4_PREFIX/bin/geant4.sh"
fi
export CMAKE_PREFIX_PATH="$GEANT4_PREFIX:${CMAKE_PREFIX_PATH:-}"
export LD_LIBRARY_PATH="$GEANT4_PREFIX/lib:${LD_LIBRARY_PATH:-}"

# Human-readable label of the *resolved* Geant4 install (from the prefix path),
# so it stays accurate even when GEANT4_VERSION_NAME differs from what was picked.
G4_LABEL="$(basename "$(dirname "$GEANT4_PREFIX")")"
[[ -n "$G4_LABEL" && "$G4_LABEL" != "." ]] || G4_LABEL="${GEANT4_VERSION_NAME:-unknown}"

BUILD_DIR="${BUILD_DIR:-$ROOT/build}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
JOBS="${JOBS:-$(nproc)}"
BIN="$BUILD_DIR/expLRB"
STAMP="$BUILD_DIR/.geant4_prefix"

if [[ "${clean:-0}" == "1" ]]; then
  echo "[build] clean: removing $BUILD_DIR"
  rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"

# --- fast path: nothing newer than the binary ---
if [[ -x "$BIN" && -f "$BUILD_DIR/CMakeCache.txt" ]]; then
  newer="$(find "$ROOT/src" "$ROOT/include" "$ROOT/main.cc" "$ROOT/CMakeLists.txt" \
             -newer "$BIN" -print -quit 2>/dev/null || true)"
  if [[ -z "$newer" ]]; then
    echo "[build] up-to-date: $BIN (Geant4: $G4_LABEL)"
    exit 0
  fi
  echo "[build] changes detected (e.g. ${newer#$ROOT/})"
fi

# --- reconfigure if needed ---
prev_prefix="$(cat "$STAMP" 2>/dev/null || true)"
need_configure=0
[[ -f "$BUILD_DIR/CMakeCache.txt" ]] || need_configure=1
[[ "$prev_prefix" != "$GEANT4_PREFIX" ]] && need_configure=1
[[ -f "$ROOT/CMakeLists.txt" && -f "$BUILD_DIR/CMakeCache.txt" && "$ROOT/CMakeLists.txt" -nt "$BUILD_DIR/CMakeCache.txt" ]] && need_configure=1

if [[ "$need_configure" == "1" ]]; then
  echo "[build] configuring (Geant4 prefix: $GEANT4_PREFIX)"
  rm -f "$BUILD_DIR/CMakeCache.txt"
  cmake -S "$ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
  echo "$GEANT4_PREFIX" > "$STAMP"
fi

echo "[build] compiling with $JOBS job(s) ..."
cmake --build "$BUILD_DIR" -j "$JOBS"
echo "[build] done: $BIN"
