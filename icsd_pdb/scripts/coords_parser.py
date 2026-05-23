# coords_parser.py
from Bio.PDB import MMCIFParser
import pathlib
import pandas as pd

from config import *

histone_chain = {"A", "B", "C", "D", "E", "F", "G", "H"}

def parser(name):
    """
    Парсер CIF на основе Biopython.
    """
    # Подтягиваем пути из твоего словаря molecules (как в твоем коде)
    input_path = pathlib.Path(molecules[name]["cif"])
    if not(input_path.exists()):
        print(f"CIF file: {input_path} don't exists!")
        return
    output_path = pathlib.Path(molecules[name]["csv"])
    if output_path.exists():
        print(f"CSV file: {output_path} already exists!")
        return

    parser = MMCIFParser(QUIET=True) # QUIET=True убирает мелкие предупреждения о пропусках
    
    try:
        # Загружаем структуру
        # name — это ID, input_path — путь к файлу
        structure = parser.get_structure(name, str(input_path))
        
        atoms = []
        average_x = average_y = average_z = 0
        count = 0
        uni_atoms = set()

        # 1. Первый проход: считаем среднее (как в твоем коде)
        # Biopython позволяет легко итерироваться по всем атомам
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

        # 2. Второй проход: собираем данные
        # Итерируемся по моделям -> цепочкам -> остаткам -> атомам
        for model in structure:
            for chain in model:
                chain_id = chain.get_id()
                for residue in chain:
                    for atom in residue:
                        element = atom.element # Biopython сам определяет элемент (C, N, O...)
                        if len(element) > 1 and element not in ["CL", "MG", "FE", "ZN", "MN", "CA", "NA"]:
                            element = element[0]
                        coord = atom.get_coord()
                        
                        # Определяем, пустой атом или гистон (твоя логика)
                        if chain_id in molecules[name]["chain"]:
                            continue
                        else:
                            element_label = element
                        
                        atoms.append([
                            element_label, 
                            coord[0] - average_x, 
                            coord[1] - average_y, 
                            coord[2] - average_z
                        ])
                        uni_atoms.add(element)

        # Сохранение в CSV
        df_out = pd.DataFrame(atoms, columns=['element', 'x', 'y', 'z'])
        df_out.to_csv(output_path, index=False)
        
        print(f"Average: {average_x:.2f} | {average_y:.2f} | {average_z:.2f}")
        print(f"Unique elements: {uni_atoms}")
        
    except Exception as e:
        print(f"Error at parsing {name}: {e}")