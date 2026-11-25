#!/usr/bin/env bash
# Generate the expLRB run matrix from config/runs.conf.
#
# Input : config/runs.conf
# Output: config/macros/<scenario>.mac
#         config/macros/manifest.tsv   (scenario / refs / events / threads / macro)
#         config/plot_scenarios.inc    (C++ initializer for plot.C AnalysisConfig)
#
# The parser is strict on purpose: unknown section references, missing required
# fields and non-numeric counts fail loudly with the offending section name.
# Re-running on an unchanged config is idempotent (output is deterministic).
#
# Usage: scripts/gen_macros.sh [path/to/runs.conf]
set -euo pipefail

SELF_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SELF_DIR/.." && pwd)"
CONF="${1:-$ROOT/config/runs.conf}"
OUT="$ROOT/config/macros"
MANIFEST="$OUT/manifest.tsv"
PLOT_INC="$ROOT/config/plot_scenarios.inc"

die() { echo "[gen_macros] ERROR: $*" >&2; exit 1; }

trim() {
  local s="$1"
  s="${s#"${s%%[![:space:]]*}"}"
  s="${s%"${s##*[![:space:]]}"}"
  printf '%s' "$s"
}

is_int() { [[ "$1" =~ ^[0-9]+$ ]]; }
is_num() { [[ "$1" =~ ^[0-9]+([.][0-9]+)?([eE][-+]?[0-9]+)?$ ]]; }

# ---------------------------------------------------------------------------
# Parser: fills
#   sections[]            ordered list of "type:name"
#   SEQ["type:name|i"]    ordered "key<TAB>value" entries of a section
#   NSEQ["type:name"]     number of SEQ entries
#   VAL["type:name|key"]  last value of a scalar key
# ---------------------------------------------------------------------------
declare -a sections=()
declare -A SEQ NSEQ VAL SECSEEN

parse_conf() {
  local conf="$1" raw line inner type name key val cur="" ln=0 i
  [[ -f "$conf" ]] || die "config file not found: $conf"
  while IFS= read -r raw || [[ -n "$raw" ]]; do
    ln=$((ln + 1))
    raw="${raw%$'\r'}"
    line="$(trim "$raw")"
    [[ -z "$line" ]] && continue
    [[ "${line:0:1}" == "#" ]] && continue
    if [[ "${line:0:1}" == "[" ]]; then
      [[ "${line: -1}" == "]" ]] || die "line $ln: malformed section header: $line"
      inner="${line:1:${#line}-2}"
      if [[ "$inner" == *:* ]]; then
        type="$(trim "${inner%%:*}")"
        name="$(trim "${inner#*:}")"
      else
        type="$(trim "$inner")"
        name=""
      fi
      [[ -n "$type" ]] || die "line $ln: empty section type"
      cur="$type:$name"
      if [[ -z "${SECSEEN["$cur"]:-}" ]]; then
        SECSEEN["$cur"]=1
        sections+=("$cur")
      fi
      continue
    fi
    [[ "$line" == *"="* ]] || die "line $ln: expected 'key = value' or '[section]': $line"
    key="$(trim "${line%%=*}")"
    val="$(trim "${line#*=}")"
    [[ -n "$cur" ]] || die "line $ln: key '$key' appears outside any section"
    [[ -n "$key" ]] || die "line $ln: empty key"
    i="${NSEQ["$cur"]:-0}"
    SEQ["$cur|$i"]="$key"$'\t'"$val"
    NSEQ["$cur"]=$((i + 1))
    VAL["$cur|$key"]="$val"
  done < "$conf"
}

getv()  { printf '%s' "${VAL["$1|$2"]:-}"; }
nseq()  { printf '%s' "${NSEQ["$1"]:-0}"; }
seqat() { printf '%s' "${SEQ["$1|$2"]:-}"; }

has_section() { [[ -n "${SECSEEN["$1"]:-}" ]]; }

# field <section> <key> <scenario>: scenario override, else referenced profile.
field() {
  local sec="$1" key="$2" sc="$3"
  if [[ -n "${VAL["scenario:$sc|$sec.$key"]+x}" ]]; then
    printf '%s' "${VAL["scenario:$sc|$sec.$key"]}"
    return
  fi
  if [[ "$sec" == "run" && -n "${VAL["scenario:$sc|$key"]+x}" ]]; then
    printf '%s' "${VAL["scenario:$sc|$key"]}"
    return
  fi
  local ref
  ref="$(getv "scenario:$sc" "$sec")"
  printf '%s' "${VAL["$sec:$ref|$key"]:-}"
}

require_field() {
  local sc="$1" sec="$2" key="$3" val
  val="$(field "$sec" "$key" "$sc")"
  [[ -n "$val" ]] || die "scenario '$sc': required field '$sec.$key' is missing"
}

# ---------------------------------------------------------------------------
# Emitters
# ---------------------------------------------------------------------------
emit_materials() {
  local sec="$1" n i entry key val
  [[ -z "$sec" || "$sec" == "none" ]] && return 0
  has_section "materials:$sec" || die "unknown materials profile: $sec"
  n="$(nseq "materials:$sec")"
  for ((i = 0; i < n; i++)); do
    entry="$(seqat "materials:$sec" "$i")"
    key="${entry%%$'\t'*}"
    val="${entry#*$'\t'}"
    case "$key" in
      create)
        set -- $val
        [[ "$#" -ge 4 ]] || die "materials:$sec: 'create' needs '<name> <density> <unit> <n>' (got: $val)"
        is_num "$2" || die "materials:$sec: non-numeric density in '$val'"
        is_int "$4" || die "materials:$sec: non-integer component count in '$val'"
        echo "/expLRB/material/create $val"
        ;;
      add)
        set -- $val
        [[ "$#" -ge 2 ]] || die "materials:$sec: 'add' needs '<element> <fraction>' (got: $val)"
        is_num "$2" || die "materials:$sec: non-numeric fraction in '$val'"
        echo "/expLRB/material/add $val"
        ;;
    esac
  done
}

emit_layers() {
  local sec="$1" n i entry key val
  has_section "geometry:$sec" || die "unknown geometry profile: $sec"
  n="$(nseq "geometry:$sec")"
  for ((i = 0; i < n; i++)); do
    entry="$(seqat "geometry:$sec" "$i")"
    key="${entry%%$'\t'*}"
    val="${entry#*$'\t'}"
    [[ "$key" == "layer" ]] || continue
    echo "/expLRB/det/addLayer $val"
  done
}

emit_macro() {
  local sc="$1"
  local geom sample phys scor run beam mat
  geom="$(getv "scenario:$sc" geometry)"
  sample="$(getv "scenario:$sc" sample)"
  phys="$(getv "scenario:$sc" physics)"
  scor="$(getv "scenario:$sc" scoring)"
  run="$(getv "scenario:$sc" run)"
  beam="$(getv "scenario:$sc" beam)"
  mat="$(getv "scenario:$sc" materials)"

  [[ -n "$geom" ]]   || die "scenario '$sc': missing 'geometry' reference"
  [[ -n "$sample" ]] || die "scenario '$sc': missing 'sample' reference"
  [[ -n "$phys" ]]   || die "scenario '$sc': missing 'physics' reference"
  [[ -n "$run" ]]    || die "scenario '$sc': missing 'run' reference"
  [[ -n "$beam" ]]   || die "scenario '$sc': missing 'beam' reference"
  [[ -n "$scor" ]]   || die "scenario '$sc': missing 'scoring' reference"

  for pair in "geometry:$geom" "sample:$sample" "physics:$phys" "scoring:$scor" "run:$run" "beam:$beam"; do
    has_section "$pair" || die "scenario '$sc': referenced section '[$pair]' does not exist"
  done
  [[ -z "$mat" || "$mat" == "none" ]] || has_section "materials:$mat" \
    || die "scenario '$sc': referenced materials profile '$mat' does not exist"

  for k in material cells size pos layer-physics score-target; do require_field "$sc" sample "$k"; done
  for k in hadronic hadronic-enabled em dna-activator; do require_field "$sc" physics "$k"; done
  for k in events threads print-progress; do require_field "$sc" run "$k"; done
  for k in particle ion energy; do require_field "$sc" beam "$k"; done
  require_field "$sc" scoring profile

  local out label override
  out="$(getv "scenario:$sc" output)"
  [[ -n "$out" ]] || out="output/root/$sc.root"
  label="$(getv "scenario:$sc" label)"
  [[ -n "$label" ]] || label="$sc"
  override="$(getv "scenario:$sc" override)"

  local cells threads events progress
  cells="$(field sample cells "$sc")"
  threads="$(field run threads "$sc")"
  events="$(field run events "$sc")"
  progress="$(field run print-progress "$sc")"
  is_int "$cells"    || die "scenario '$sc': sample.cells must be a non-negative integer (got: $cells)"
  is_int "$threads"  || die "scenario '$sc': run.threads must be a positive integer (got: $threads)"
  is_int "$events"   || die "scenario '$sc': run.events must be a positive integer (got: $events)"
  is_int "$progress" || die "scenario '$sc': run.print-progress must be a positive integer (got: $progress)"
  [[ "$threads" -ge 1 ]] || die "scenario '$sc': run.threads must be >= 1"
  [[ "$events" -ge 1 ]]  || die "scenario '$sc': run.events must be >= 1"

  {
    echo "# AUTO-GENERATED by scripts/gen_macros.sh from config/runs.conf — edit the config, not this file."
    echo "# scenario=$sc geometry=$geom sample=$sample physics=$phys scoring=$scor run=$run beam=$beam"
    echo "/control/verbose 1"
    echo "/run/verbose 1"
    echo "/run/numberOfThreads $threads"
    echo "/run/printProgress $progress"
    echo "# --- Physics (PreInit) ---"
    echo "/physics/setHadronic $(field physics hadronic "$sc")"
    echo "/physics/enableHadronic $(field physics hadronic-enabled "$sc")"
    echo "/physics/setEM $(field physics em "$sc")"
    echo "/physics/enableDNAActivator $(field physics dna-activator "$sc")"
    echo "# --- Output and target ---"
    echo "/bio/output/fileName $out"
    echo "/bio/score/profile $(field scoring profile "$sc")"
    echo "/bio/score/target $(field sample score-target "$sc")"
    echo "# --- World ---"
    [[ -n "$(field geometry world-mat "$sc")" ]] && echo "/expLRB/det/setWorldMat $(field geometry world-mat "$sc")"
    [[ -n "$(field geometry world-xy "$sc")" ]] && echo "/expLRB/det/setWorldXY $(field geometry world-xy "$sc")"
    [[ -n "$(field geometry world-z "$sc")" ]] && echo "/expLRB/det/setWorldZ $(field geometry world-z "$sc")"
    [[ -n "$(field geometry prebio-cut "$sc")" ]] && echo "/expLRB/det/setPreBioCut $(field geometry prebio-cut "$sc")"
    echo "# --- Materials (LRB JINR data) ---"
    emit_materials "$mat"
    echo "# --- Beam tract ---"
    emit_layers "$geom"
    echo "# --- Biolayer ---"
    if [[ "$cells" -gt 0 ]]; then
      echo "/bio/cell/geometry $(field sample cell-geometry "$sc")"
      echo "/bio/cell/materials $(field sample cell-materials "$sc")"
      echo "/bio/cell/densities $(field sample cell-densities "$sc")"
      echo "/bio/cell/physics $(field sample cell-physics "$sc")"
    fi
    echo "/bio/layer/create $(field sample material "$sc") $cells"
    echo "/bio/layer/size $(field sample size "$sc")"
    echo "/bio/layer/pos $(field sample pos "$sc")"
    echo "/bio/layer/physics $(field sample layer-physics "$sc")"
    [[ -n "$override" ]] && echo "/control/execute $override"
    echo "# --- Beam ---"
    echo "/run/initialize"
    echo "/gun/particle $(field beam particle "$sc")"
    echo "/gun/ion $(field beam ion "$sc")"
    echo "/gun/energy $(field beam energy "$sc")"
    [[ -n "$(field beam radius "$sc")" ]] && echo "/expLRB/gun/setRadius $(field beam radius "$sc")"
    echo "# Event count is resolved by /bio/run/beamOn (override with --events N)"
    echo "/bio/run/setEvents $events"
    echo "/bio/run/beamOn"
  } > "$OUT/$sc.mac"
  echo "[gen_macros] wrote $sc.mac"
}

# ---------------------------------------------------------------------------
# plot.C scenario table (config/plot_scenarios.inc)
# ---------------------------------------------------------------------------
PLOT_PALETTE=(kBlue kRed kGreen kMagenta kOrange kCyan kViolet kPink)

plot_scenarios_inc() {
  local -a scen=()
  local s
  for s in "${sections[@]}"; do
    [[ "$s" == scenario:* ]] && scen+=("${s#scenario:}")
  done
  local i=0 tag
  {
    echo "// AUTO-GENERATED by scripts/gen_macros.sh from config/runs.conf — do not edit."
    echo "// Entries for AnalysisConfig::scenarios in plot.C."
  } > "$PLOT_INC"
  for tag in "${scen[@]}"; do
    local out label cells r rho color
    out="$(getv "scenario:$tag" output)"
    [[ -n "$out" ]] || out="output/root/$tag.root"
    label="$(getv "scenario:$tag" label)"
    [[ -n "$label" ]] || label="$tag"
    cells="$(field sample cells "$tag")"
    r="$(field sample cell-geometry "$tag")"; r="${r%% *}"; [[ -n "$r" ]] || r="3.1"
    rho="$(field sample cell-densities "$tag")"; rho="${rho%% *}"; [[ -n "$rho" ]] || rho="1.0"
    color="$(getv "scenario:$tag" color)"
    if [[ -z "$color" ]]; then
      color="${PLOT_PALETTE[$((i % ${#PLOT_PALETTE[@]}))]} + 1"
    fi
    {
      echo "        {"
      echo "            \"$tag\","
      echo "            \"$label\","
      echo "            \"$out\","
      echo "            ${cells}.0, $r, $rho,"
      echo "            $color, 1"
      echo "        },"
    } >> "$PLOT_INC"
    i=$((i + 1))
  done
}

# ---------------------------------------------------------------------------
main() {
  mkdir -p "$OUT"
  parse_conf "$CONF"

  local s
  local -a scen=()
  for s in "${sections[@]}"; do
    [[ "$s" == scenario:* ]] && scen+=("${s#scenario:}")
  done
  [[ "${#scen[@]}" -gt 0 ]] || die "no [scenario:...] sections found in $CONF"

  {
    printf 'scenario\tgeometry\tsample\tphysics\tscoring\tevents\tthreads\tmacro\tmemory\tdisk\n'
    local tag
    for tag in "${scen[@]}"; do
      printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\tconfig/macros/%s.mac\t%s\t%s\n' \
        "$tag" \
        "$(getv "scenario:$tag" geometry)" \
        "$(getv "scenario:$tag" sample)" \
        "$(getv "scenario:$tag" physics)" \
        "$(getv "scenario:$tag" scoring)" \
        "$(field run events "$tag")" \
        "$(field run threads "$tag")" \
        "$tag" \
        "$(field sample memory "$tag")" \
        "$(field sample disk "$tag")"
    done
  } > "$MANIFEST"

  for tag in "${scen[@]}"; do emit_macro "$tag"; done
  plot_scenarios_inc
  echo "[gen_macros] ${#scen[@]} scenarios -> $OUT  (manifest: $MANIFEST)"
}

main "$@"
