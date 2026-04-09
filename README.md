# Geant4-DNA: Microdosimetry & Ion Transport Validation ⚛️🧬

This repository contains research data, simulation scripts, and analysis tools developed during my research stays at the **Joint Institute for Nuclear Research (JINR)** in Dubna.

## 🌟 Overview
The project focuses on the validation of ion transport models within the `Geant4-DNA` sub-package, aiming to establish a computational link between physical energy deposition and biological DNA damage.

---

## 🛰️ Research Phases

### 1️⃣ START Programme (Summer Session 2025)
**Focus:** Radial Dose Distribution & LET Validation.
- **Task:** Validation of Radial Dose Distribution for protons and heavy ions (He, Li, C, Ne) using Geant4 11.3.
- **Clustering:** Implemented **K-Means**, **Genetic-K-Means**, and **DBSCAN** algorithms to analyze secondary particle distributions.
- **Outcome:** Benchmarked energy deposition patterns against experimental data.

### 2️⃣ INTEREST Programme (Wave 13)
**Focus:** Ionisation Cluster Size Distribution (ICSD).
- **Task:** Validation of ICSD for a wide range of ions (Proton to Carbon).
- **Physics Optimization:** Proved that standard EM physics lists are unsuitable for DNA-scale modeling; identified `G4EmDNAPhysics_option6` as the most accurate.
- **Visualization:** Developed Python tools to visualize transverse track structures (**Core vs. Penumbra**).

### 3️⃣ INTEREST Programme (Wave 14 / Spring 2026)
**Focus:** Synthesis & Scientific Publication.
- **Task:** Benchmarking Range, Detour factor, and new Stopping Power calculations.
- **Goal:** Integration of START and INTEREST results into a unified validation framework for the *Medical Physics* journal.

---

## 🛠 Tech Stack
- **Simulation:** `Geant4` (v11.3, 11.4) / `C++`
- **Analysis:** `Python` (NumPy, SciPy, Pandas)
- **ML/Clustering:** `ROOT` (DBSCAN, K-Means)
- **Visualization:** `Matplotlib`, `ROOT` (for 3D track structures)

---

## 📊 Key Results
- **Optimization:** Achieved a **2x simulation speedup** by optimizing the Rudd Ionization model parameters.
- **Accuracy:** Validated low-energy transitions ensuring smooth data handover between relativistic and microdosimetric models.

---

## 📂 Repository Structure
```text
├── INTEREST ├── icsd
             ├── range
             ├── spower
├── START ├── ionrange
          ├── ionradial
