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

# emit_macro <scenario-ref> [tag] [hadronic-override] [output-override]
#   <scenario-ref> is the [scenario:...] whose profiles are used;
#   [tag]      the macro/tag name (defaults to <scenario-ref>; for axis variants
#              it is <scenario-ref>__<had>);
#   had override replaces /physics/setHadronic (axis variants);
#   output override replaces /bio/output/fileName (defaults to output/root/<tag>.root).
emit_macro() {
  local sc="$1" tag="${2:-$1}" had_override="${3:-}" out_override="${4:-}"
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
  for k in events threads; do require_field "$sc" run "$k"; done
  for k in particle ion energy; do require_field "$sc" beam "$k"; done
  require_field "$sc" scoring profile

  local out label override
  if [[ -n "$out_override" ]]; then
    out="$out_override"
  else
    out="$(getv "scenario:$sc" output)"
    [[ -n "$out" ]] || out="output/root/$sc.root"
  fi
  label="$(getv "scenario:$sc" label)"
  [[ -n "$label" ]] || label="$sc"
  override="$(getv "scenario:$sc" override)"

  local cells threads events progress progress_lines
  cells="$(field sample cells "$sc")"
  threads="$(field run threads "$sc")"
  events="$(field run events "$sc")"
  is_int "$cells"    || die "scenario '$sc': sample.cells must be a non-negative integer (got: $cells)"
  is_int "$threads"  || die "scenario '$sc': run.threads must be a positive integer (got: $threads)"
  is_int "$events"   || die "scenario '$sc': run.events must be a positive integer (got: $events)"
  [[ "$threads" -ge 1 ]] || die "scenario '$sc': run.threads must be >= 1"
  [[ "$events" -ge 1 ]]  || die "scenario '$sc': run.events must be >= 1"

  # print-progress: explicit value wins; otherwise derive it from the run size so the
  # number of progress lines stays bounded (~progress-lines, default 500) regardless of
  # how many events are simulated, and never finer than every 10 events.
  progress="$(field run print-progress "$sc")"
  if [[ -n "$progress" ]]; then
    is_int "$progress" && [[ "$progress" -ge 1 ]] || die "scenario '$sc': run.print-progress must be a positive integer (got: $progress)"
  else
    progress_lines="$(field run progress-lines "$sc")"
    [[ -n "$progress_lines" ]] || progress_lines=500
    is_int "$progress_lines" && [[ "$progress_lines" -ge 1 ]] || die "scenario '$sc': run.progress-lines must be a positive integer (got: $progress_lines)"
    progress=$(( (events + progress_lines - 1) / progress_lines ))
    (( progress < 10 )) && progress=10
  fi

  {
    echo "# AUTO-GENERATED by scripts/gen_macros.sh from config/runs.conf — edit the config, not this file."
    echo "# scenario=$sc geometry=$geom sample=$sample physics=$phys scoring=$scor run=$run beam=$beam"
    [[ -n "$had_override" ]] && echo "# axis=hadronic tag=$tag preset=$had_override base=$sc"
    echo "/control/verbose 1"
    echo "/run/verbose 1"
    echo "/run/numberOfThreads $threads"
    echo "/run/printProgress $progress"
    echo "# --- Physics (PreInit) ---"
    echo "/physics/setHadronic ${had_override:-$(field physics hadronic "$sc")}"
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
  } > "$OUT/$tag.mac"
  echo "[gen_macros] wrote $tag.mac"
}

# ---------------------------------------------------------------------------
# plot.C scenario table (config/plot_scenarios.inc)
#   plot_scenarios_inc <entry>...   each entry is "tag|base|had"
#     tag  - scenario tag / output name (axis variants: <base>__<had>)
#     base - the [scenario:...] whose profiles define the geometry/beam; for a
#            plain scenario base == tag; the grid groups variants by base;
#     had  - hadronic preset for axis variants, empty for plain scenarios.
# ---------------------------------------------------------------------------
PLOT_PALETTE=(kBlue kRed kGreen kMagenta kOrange kCyan kViolet kPink)

plot_scenarios_inc() {
  local -a entries=("$@")
  local i=0 entry tag base had
  {
    echo "// AUTO-GENERATED by scripts/gen_macros.sh from config/runs.conf — do not edit."
    echo "// Entries for AnalysisConfig::scenarios in plot.C."
    echo "// Fields: tag, label, rootFile, nCells, rNucleus, rho, color, lineStyle, base, had"
  } > "$PLOT_INC"
  for entry in "${entries[@]}"; do
    IFS='|' read -r tag base had <<< "$entry"
    local out label cells r rho color
    if [[ -n "$had" ]]; then
      out="output/root/$tag.root"
    else
      out="$(getv "scenario:$tag" output)"
      [[ -n "$out" ]] || out="output/root/$tag.root"
    fi
    label="$(getv "scenario:$base" label)"
    [[ -n "$label" ]] || label="$base"
    [[ -n "$had" ]] && label="$label [$had]"
    cells="$(field sample cells "$base")"
    r="$(field sample cell-geometry "$base")"; r="${r%% *}"; [[ -n "$r" ]] || r="3.1"
    rho="$(field sample cell-densities "$base")"; rho="${rho%% *}"; [[ -n "$rho" ]] || rho="1.0"
    color="$(getv "scenario:$base" color)"
    if [[ -z "$color" ]]; then
      color="${PLOT_PALETTE[$((i % ${#PLOT_PALETTE[@]}))]} + 1"
    fi
    {
      echo "        {"
      echo "            \"$tag\","
      echo "            \"$label\","
      echo "            \"$out\","
      echo "            ${cells}.0, $r, $rho,"
      echo "            $color, 1,"
      echo "            \"$base\", \"$had\""
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

  # --- hadronic configuration axis (optional) ------------------------------
  local -a had_presets=()
  local axis_enabled=0
  if has_section "axis:hadronic"; then
    case "$(getv "axis:hadronic" enabled)" in
      true|1|yes|on) axis_enabled=1 ;;
    esac
    if [[ "$axis_enabled" == "1" ]]; then
      read -r -a had_presets <<< "$(getv "axis:hadronic" presets)"
    fi
  fi
  if [[ "$axis_enabled" == "1" && "${#had_presets[@]}" -eq 0 ]]; then
    die "[axis:hadronic] enabled but 'presets' is empty"
  fi
  local preset
  for preset in "${had_presets[@]}"; do
    case "$preset" in
      QGSP_BIC_HP|QGSP_BIC|QGSP_BERT_HP|FTF_BIC|Shielding|FTFP_BERT_HP|QBBC|INCLXX_HP) ;;
      *) die "[axis:hadronic] unknown preset '$preset' (see /physics/setHadronic candidates)" ;;
    esac
  done

  # A scenario is swept iff the axis is on, its physics profile has hadronic
  # enabled, and it does not opt out with `had-axis = false`.
  axis_sweeps() {
    local sc="$1"
    [[ "$axis_enabled" == "1" ]] || return 1
    [[ "$(getv "scenario:$sc" had-axis)" == "false" ]] && return 1
    [[ "$(field physics hadronic-enabled "$sc")" == "true" ]] || return 1
    return 0
  }

  # Ordered plot/manifest entries: "tag|base|had".
  local -a entries=()
  local tag base lc
  for base in "${scen[@]}"; do
    entries+=("${base}|${base}|")
    if axis_sweeps "$base"; then
      for preset in "${had_presets[@]}"; do
        lc="$(printf '%s' "$preset" | tr '[:upper:]' '[:lower:]')"
        entries+=("${base}__${lc}|${base}|${preset}")
      done
    fi
  done

  {
    printf 'scenario\tgeometry\tsample\tphysics\tscoring\tevents\tthreads\tmacro\tmemory\tdisk\tbase\thad\n'
    local entry
    for entry in "${entries[@]}"; do
      IFS='|' read -r tag base had <<< "$entry"
      printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\tconfig/macros/%s.mac\t%s\t%s\t%s\t%s\n' \
        "$tag" \
        "$(getv "scenario:$base" geometry)" \
        "$(getv "scenario:$base" sample)" \
        "$(getv "scenario:$base" physics)" \
        "$(getv "scenario:$base" scoring)" \
        "$(field run events "$base")" \
        "$(field run threads "$base")" \
        "$tag" \
        "$(field sample memory "$base")" \
        "$(field sample disk "$base")" \
        "$base" \
        "$had"
    done
  } > "$MANIFEST"

  local n_axis=0
  for entry in "${entries[@]}"; do
    IFS='|' read -r tag base had <<< "$entry"
    if [[ -n "$had" ]]; then
      emit_macro "$base" "$tag" "$had" "output/root/$tag.root"
      n_axis=$((n_axis + 1))
    else
      emit_macro "$base"
    fi
  done

  plot_scenarios_inc "${entries[@]}"
  echo "[gen_macros] ${#scen[@]} scenarios + ${n_axis} axis variants (presets: ${had_presets[*]:-none}) -> $OUT"
  echo "[gen_macros] manifest: $MANIFEST"
}

main "$@"
