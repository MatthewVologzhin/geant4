```markdown
# Comparative Analysis of Ionisation Cluster Size Distributions (ICSD) in Sub-Cellular Structures using Geant4-DNA

This Geant4-DNA application (`icsd`) simulates and evaluates **Ionisation Cluster Size Distributions (ICSD)** within nanometer-scale sub-cellular and macromolecular biological volumes (e.g., DNA, nucleosomes, chromatin models, cell structures). 

The primary objective is to systematically compare different geometry construction methods in Geant4-DNA and assess their impact on physical accuracy and computational overhead when simulating radiation effects in space planning and hadron/hadron-equivalent radiation therapy.

---

## Table of Contents
1. [Scientific Background & Purpose](#1-scientific-background--purpose)
2. [Project Architecture](#2-project-architecture)
3. [Geant4 UI Commands (Macro Reference)](#3-geant4-ui-commands-macro-reference)
4. [Script Reference](#4-script-reference)
5. [Prerequisites & Installation](#5-prerequisites--installation)
6. [How to Run](#6-how-to-run)
7. [Results & Visualization](#7-results--visualization)
8. [Scientific Conclusions](#8-scientific-conclusions)
9. [References](#9-references)

---

## 1. Scientific Background & Purpose

Predicting radiation-induced DNA damage requires highly precise nanometer-scale calculations. Detailed track-structure simulations are computationally expensive, which forces developers to use geometric model approximations. 

This project implements and evaluates three different geometric representation methods inside Geant4-DNA:
1. **Predefined Geometries:** Simplified cylindrical (PTB, StarTrack, Cytoskeleton, NMDA) or spherical (Histone, Ribosome) targets. Extremely fast but geometrically idealized.
2. **Voxelized Geometries:** Splitting realistic macromolecular atomic coordinates (parsed from PDB/CIF) into a regular grid of non-overlapping voxel cubes with size $d$ (e.g., $1\,\text{Å}, 5\,\text{Å}, 10\,\text{Å}$).
3. **Boolean Geometries:** Constructing complex exact shapes by recursively applying union operations (`G4UnionSolid` balanced binary trees) to spheres representing individual atoms, placed at their exact physical coordinates using van der Waals radii (Source: Alvarez, 2013).

The simulation computes the stochastic clustering of ionisations inside these nanometric volumes under different track conditions (using light and heavy ions: $p, d, \alpha, {}^{6}\text{Li}, {}^{7}\text{Li}, {}^{12}\text{C}$ over various energies) to evaluate the threshold-level accuracy and performance of each geometry approach.

---

## 2. Project Architecture

The project is structured according to best practices for cluster execution, automated dependencies isolation, and modular design:

```text
icsd_pdb/
├── CMakeLists.txt              # CMake build configuration for C++
├── requirements.txt            # Python dependencies (Biopython, Plotly, Pandas, etc.)
├── parameters.txt              # Parameter scan card for HTCondor jobs
├── project_config.sh           # Global variables (Geant4 paths, compilation settings)
├── run.sh                      # Unified launch orchestrator (venv setup, parsing, compilation)
├── analysis.sh                 # Automation script for running ROOT plotting macros
├── README.md                   # Project documentation
├── icsd                        # Compiled Geant4 executable
│
├── include/                    # C++ Header files (.hh)
│   ├── ActionInitialization.hh
│   ├── DetectorConstruction.hh
│   ├── DetectorMessenger.hh
│   ├── EventAction.hh
│   ├── PhysicsList.hh
│   ├── PrimaryGeneratorAction.hh
│   ├── RunAction.hh
│   ├── RunMessenger.hh
│   ├── StackingAction.hh
│   └── SteppingAction.hh
│
├── src/                        # C++ Source files (.cc)
│   ├── ActionInitialization.cc
│   ├── DetectorConstruction.cc
│   ├── DetectorMessenger.cc
│   ├── EventAction.cc
│   ├── PhysicsList.cc
│   ├── PrimaryGeneratorAction.cc
│   ├── RunAction.cc
│   ├── RunMessenger.cc
│   ├── StackingAction.cc
│   └── SteppingAction.cc
│
├── jobs/                       # HTCondor cluster automation configurations
│   └── geant4.sub              # HTCondor submit description file
│
├── scripts/                    # Preprocessing, analysis, and automation scripts
│   ├── root/                   # ROOT analysis macros (.C)
│   │   ├── plotGeometryComparison.C
│   │   ├── plotGeometryComparisonTime.C
│   │   ├── plotTimeComparison.C
│   │   └── plotValidation.C
│   ├── config.py               # Constants, Alvarez radii mapping, and PDB definitions
│   ├── coords_parser.py        # Biopython parser converting CIF to CSV coordinates
│   ├── visualize.py            # Plotly-based interactive 3D macromolecular visualizer
│   ├── main.py                 # Download and pre-process coordinate data
│   ├── compile_fast.sh         # Fast C++ compilation script using local SSD /tmp space
│   ├── setup_venv.sh           # Automated Python virtual environment manager
│   └── run_geant4.sh           # Worker-node execution shell script
│
├── input/                      # Unalterable input datasets
│   ├── experimental_data/      # Experimental data of the PTB and the StarTrack experiments
│   └── cif/                    # Crystallographic Information Files (.cif) of macromolecules
│  
│
└── output/                     # Automatically generated output files (ignored by Git)
    ├── csv/                    # Extracted atomic coordinates from CIF files
    ├── root/                   # Generated Geant4 output files (*.root)
    ├── plots/                  # Rendered HTML visualizations and plots
    └── logs/                   # Standard output and error logs from Condor runs
```

---

## 3. Geant4 UI Commands (Macro Reference)

Custom UI messengers configure the simulation setup at runtime using macro commands.

### 3.1 Detector Configuration Commands (`/det/`)
* `/det/setGeom <Name>`
  Sets the target biological geometry. Predefined types configure analytical cylinders or spheres. Any other string loads coordinate datasets from a matching CSV in the data directory.
  * *Candidates:* `PTB` | `StarTrack` | `Cytoskeleton` | `Ribosome` | `NMDA` | `Histone` | *(any PDB code)*
* `/det/setGeomMethod <Method>`
  Specifies the geometrical rendering approach for biological target shapes.
  * *Candidates:* `VOXEL` | `BOOLEAN` | `PREDEFINED`
* `/det/setDiameter <Double> <Unit>`
  Sets the target cylinder/sphere diameter.
* `/det/setHeight <Double> <Unit>`
  Sets the height of the cylindrical targets.
* `/det/setVoxelSize <Double> <Unit>`
  Configures the voxel grid size used if the `VOXEL` geometry method is selected.
* `/det/setDensity <Double> <Unit>`
  Sets the target density. Typically defaults to $1.0\,\text{g/cm}^3$ for water phantoms or $1.407\,\text{g/cm}^3$ for condensed macromolecular structures.
* `/det/setEfficiency <Double>`
  Models detector collection efficiency $[0.0, 1.0]$. (e.g., $1.0$ for PTB, $0.2$ for StarTrack).
* `/det/setDataDir <String>`
  Specifies the directory location containing parsed atomic coordinate tables.

### 3.2 Physics Configuration Commands (`/physics/`)
* `/physics/setPhysics <ConstructorName>`
  Changes the low-energy physics solver. Must be invoked during the `PreInit` state.
  * *Candidates:* `G4EmDNAPhysics` | `G4EmDNAPhysics_option2` | `G4EmDNAPhysics_option4` | `G4EmDNAPhysics_option6` | `G4EmDNAPhysics_option8` | `G4EmStandardPhysics_option0` | `G4EmStandardPhysics_option4` | `G4EmLivermorePhysics`

### 3.3 Output Configuration Commands (`/icsd/analysis/`)
* `/icsd/analysis/setFileName <Path/Filename>`
  Specifies the relative path and name of the generated ROOT file (e.g., `output/root/icsd_run`).

---

## 4. Script Reference

### 4.1 Orchestration & Shell Scripts
* `run.sh`
  Initializes folder directories, compiles C++ code safely, downloads and parses macromolecular datasets, and automates job submission to HTCondor with real-time tracking.
* `analysis.sh`
  Verifies ROOT environment availability and runs the ROOT macro files inside `scripts/root/` sequentially in batch mode (`-b -l -q`) to generate comparison plots.
* `scripts/compile_fast.sh`
  Detects source code changes. Uses local SSD space (`/tmp`) for fast C++ building to bypass network file system (NFS) write latency bottlenecks.
* `scripts/setup_venv.sh`
  Automates Python virtual environment configuration inside `.venv/` and pulls in isolated dependencies from `requirements.txt`.
* `scripts/run_geant4.sh`
  Generates transient macro files, configures variables (such as physics, energy, particle types, voxels), sources the local Geant4 shell paths, and handles the job cleanup.

### 4.2 Python Data & Pre-processing Scripts
* `scripts/config.py`
  Defines global directories, CPK coloring schemas, Alvarez van der Waals atomic radii, and target macromolecules properties.
* `scripts/coords_parser.py`
  Parses crystallographic MMCIF (`.cif`) files utilizing Biopython's `MMCIFParser`, adjusts atomic bounds relative to the center of mass, applies downsampling skip filters, and exports cleaned Cartesian tables into `output/csv/`.
* `scripts/visualize.py`
  Generates custom, interactive 3D visualizations of macromolecules (spheres for orbs, tubes for channels) using Plotly and saves them as HTML.
* `scripts/main.py`
  Iterates through required targets dynamically parsed from `parameters.txt`, downloads missing structures directly from the RCSB PDB database, and triggers coordinates pre-processing.

---

## 5. Prerequisites & Installation

### 5.1 C++ & Physics Solver Environments
* Geant4 (with low-energy DNA models enabled, compiled with expat).
* ROOT (installed and sourced in your system PATH).
* CMake (>= 3.16).

### 5.2 Setting up the Environment
All paths and user settings are consolidated within `project_config.sh`. Open it and modify `GEANT4_ENV` to point to your Geant4 environment configuration script:
```bash
# Inside project_config.sh
export GEANT4_ENV="/path/to/your/geant4/install/bin/geant4.sh"
```

---

## 6. How to Run

### 6.1 Run the Automated Pipeline
The easiest way to initialize the virtual environment, download and pre-process required PDB databases, compile the C++ source binary, and prepare execution is to run the unified orchestrator:
```bash
chmod +x run.sh
./run.sh
```

### 6.2 Pre-processing Coordinate Files Manually
If you want to manually run the CIF downloader and parser:
```bash
./.venv/bin/python scripts/main.py
```

### 6.3 Local Command-Line Simulation Run
Run a localized event sequence using Geant4 UI terminal or macro files:
```bash
source /path/to/geant4.sh
./icsd config/vis.mac
```

---

## 7. Results & Visualization

Every simulation execution outputs a ROOT file into `output/root/` featuring three structured Ntuples:
* `ntuple_1` (ICSD): Stochastic counts tracking overall target ionisations, primary ion-induced events, and secondary electron events.
* `ntuple_2` (physics): Tracks absolute step locations $(x,y,z)$, process type, and energy deposit $dE$. (Disabled in production for speed and disk space optimization).
* `ntuple_3` (multipleionisations): Track-structure secondary electron clusters per ionisation step.

### 7.1 Interactive 3D HTML Visualizations
Render exact structural bounds (Histone, NMDA, Ribosome, and Cytoskeleton) over their matching target volumes (orbs or cylindrical shells):
```bash
./.venv/bin/python -c "import sys; sys.path.append('scripts'); from visualize import visualize_molecules; visualize_molecules()"
```
This generates `output/plots/Macromolecules Comparison.html`, which can be viewed in any web browser.

### 7.2 Run Plotting Analysis
To sequentially execute your analytical ROOT macros and plot comparative curves:
```bash
chmod +x analysis.sh
./analysis.sh
```

---

## 8. Scientific Conclusions

* **Physical Constructors:** Validation against experimental reference data highlights `G4EmDNAPhysics_option4` as the most accurate constructor for the cylindrical **StarTrack** geometry (Hilgers 2017) and `G4EmDNAPhysics_option6` for the **PTB** experimental setup (Conte 2012).
* **Geometry Approximations:** Analytical simplified targets (cylinders and spheres) remain precise only for smaller, highly regular structures (e.g., Histone) under light ion beams ($p, d$).
* **Computational Cost:** Predefined analytical structures are the fastest. Voxelization slows down significantly with smaller voxel sizes (1 Ångstrom is the slowest).
* **Boolean Union Limitations:** While recursive binary union trees provide exact structural boundaries, they suffer from **voxel/boundary navigation bottlenecks inside Geant4's tracking core** rather than construction overhead.

---

## 9. References
1. **Incerti, S. et al.** *The Geant4-DNA Project.* Int. J. Model. Simul. Sci. Comput. 1 (2010) 157–178.
2. **Hilgers, G. et al.** *Measurement of track structure parameters of low and medium energy helium and carbon ions in nanometric volumes.* Phys. Med. Biol. 62 (2017) 7569–7597.
3. **Conte, V. et al.** *Track structure of light ions: experiments and simulations.* New J. Phys. 14 (2012) 093010.
```
