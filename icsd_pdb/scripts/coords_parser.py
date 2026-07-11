# scripts/coords_parser.py
from Bio.PDB import MMCIFParser
import pathlib
import pandas as pd

def parse_cif_to_csv(pdb_id, cif_path, csv_path, skip_val=1, excluded_chains=None):
    """
    Parses a PDB CIF file and extracts atom coordinates into a standardized CSV.
    Computes coordinates relative to the molecule's center of mass.
    """
    cif_path = pathlib.Path(cif_path)
    csv_path = pathlib.Path(csv_path)
    
    if not cif_path.exists():
        print(f"ERROR: CIF file {cif_path} does not exist!")
        return False
        
    if csv_path.exists():
        print(f"--> CSV file {csv_path} already exists. Skipping parsing.")
        return True

    # Ensure output directory exists
    csv_path.parent.mkdir(parents=True, exist_ok=True)

    if excluded_chains is None:
        excluded_chains = set()

    # QUIET=True suppresses minor PDB warnings
    parser_obj = MMCIFParser(QUIET=True)
    try:
        structure = parser_obj.get_structure(pdb_id, str(cif_path))
        atoms = []
        average_x = average_y = average_z = 0.0
        count = 0
        uni_atoms = set()

        # Step 1: Calculate center of mass (average coordinates)
        for atom in structure.get_atoms():
            coord = atom.get_coord()
            average_x += coord[0]
            average_y += coord[1]
            average_z += coord[2]
            count += 1

        if count > 0:
            average_x /= count
            average_y /= count
            average_z /= count

        atom_counter = 0
        domain_counter = 0
        domain_max = 150
        model = structure[0]

        # Step 2: Extract atom properties
        for chain in model:
            chain_id = chain.get_id()
            if chain_id in excluded_chains:
                continue
            
            domain_id = 0
            atom_inner_counter = 0
            for residue in chain:
                for atom in residue:
                    element = atom.element
                    # Clean up element name
                    if len(element) > 1 and element not in ["CL", "MG", "FE", "ZN", "MN", "CA", "NA"]:
                        element = element[0]
                    
                    coord = atom.get_coord()
                    atoms.append([
                        chain_id,
                        domain_id,
                        element,
                        coord[0] - average_x,
                        coord[1] - average_y,
                        coord[2] - average_z
                    ])
                    uni_atoms.add(element)
                    atom_inner_counter += 1
                    atom_counter += 1

                if atom_inner_counter >= domain_max:
                    domain_id += 1
                    domain_counter += 1
                    atom_inner_counter = 0

        print(f"===== Successfully parsed PDB: {pdb_id.upper()} =====")
        print(f"Atoms: {atom_counter} | Domains: {domain_counter} | Center: ({average_x:.2f}, {average_y:.2f}, {average_z:.2f})")
        
        df_out = pd.DataFrame(atoms, columns=['chain_id', 'domain_id', 'element', 'x', 'y', 'z'])
        
        # Apply downsampling (skip factor) if defined
        if skip_val > 1:
            df_out = df_out.iloc[::skip_val]
            print(f"--> Applied downsampling (skip={skip_val}). Remaining atoms: {len(df_out)}")
            
        df_out.to_csv(csv_path, index=False)
        return True

    except Exception as e:
        print(f"ERROR: Failed to parse {pdb_id.upper()}: {e}")
        return False