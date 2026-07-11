# scripts/main.py
import os
import sys
import urllib.request
import pathlib

# Import project configurations
from config import BASE_DIR, molecules
from coords_parser import parse_cif_to_csv

# Analytical geometries inside Geant4 that do not require PDB datasets
PREDEFINED_GEOMETRIES = {"ptb", "startrack", "cytoskeleton", "ribosome", "nmda", "histone"}

def download_cif_from_rcsb(pdb_id, destination_path):
    """
    Downloads a CIF file directly from the RCSB Protein Data Bank.
    """
    pdb_id = pdb_id.lower()
    url = f"https://files.rcsb.org/download/{pdb_id}.cif"
    try:
        print(f"--> Downloading {pdb_id.upper()} from RCSB PDB ({url})...")
        urllib.request.urlretrieve(url, destination_path)
        print(f"--> Successfully downloaded to {destination_path}")
        return True
    except Exception as e:
        print(f"ERROR: Failed to download {pdb_id.upper()} from RCSB PDB: {e}")
        return False

def get_geometries_from_parameters(parameters_path):
    """
    Extracts unique target geometries from the 9th column of parameters.txt.
    """
    geometries = set()
    if not os.path.exists(parameters_path):
        print(f"WARNING: Parameters file not found at {parameters_path}")
        return geometries

    with open(parameters_path, "r") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            # Split CSV parameter line
            parts = [p.strip() for p in line.split(",")]
            if len(parts) >= 9:
                geometries.add(parts[8]) # 9th column is the geometry/target name
    return geometries

def main():
    parameters_path = BASE_DIR / "parameters.txt"
    cif_dir = BASE_DIR / "input" / "cif"
    csv_dir = BASE_DIR / "output" / "csv"

    # Ensure vital directories exist
    cif_dir.mkdir(parents=True, exist_ok=True)
    csv_dir.mkdir(parents=True, exist_ok=True)

    # 1. Read required target geometries from parameters.txt at runtime
    required_geoms = get_geometries_from_parameters(parameters_path)
    print(f"--> Required geometries detected in parameters.txt: {required_geoms}")

    # 2. Process each required geometry
    for geom in required_geoms:
        geom_lower = geom.lower()
        
        # Skip analytical target models built natively by Geant4
        if geom_lower in PREDEFINED_GEOMETRIES:
            print(f"--> Geometry '{geom}' is analytical/predefined. Skipping preprocessing.")
            continue

        pdb_id = geom_lower
        skip_val = 1
        excluded_chains = set()

        # Check if the PDB target matches any configuration in config.py
        # If matched, we override default skip factors and excluded chains
        matched_config = None
        for key, val in molecules.items():
            config_pdb_id = pathlib.Path(val["cif"]).stem.lower()
            if geom_lower == config_pdb_id or geom_lower == key.lower():
                matched_config = val
                pdb_id = config_pdb_id
                skip_val = val.get("skip", 1)
                excluded_chains = val.get("chain", set())
                break

        cif_path = cif_dir / f"{pdb_id}.cif"
        csv_path = csv_dir / f"{pdb_id}.csv"

        # 3. Skip if the CSV has already been generated
        if csv_path.exists():
            print(f"--> CSV for PDB '{pdb_id.upper()}' is already up-to-date.")
            continue

        # 4. Check if CIF is present; if not -> download from RCSB PDB automatically
        if not cif_path.exists():
            success = download_cif_from_rcsb(pdb_id, cif_path)
            if not success:
                print(f"ERROR: Failed to obtain CIF for '{pdb_id.upper()}'. Skipping.")
                continue

        # 5. Convert CIF to CSV
        parse_cif_to_csv(pdb_id, cif_path, csv_path, skip_val, excluded_chains)

if __name__ == "__main__":
    main()