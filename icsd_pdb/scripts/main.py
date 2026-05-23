# main.py
import os
import sys

from config import *
from coords_parser import parser
from visualize import visualize_molecules
from check_overlaps import *

if __name__ == "__main__":
    
    path = BASE_DIR / "results" / "data"
    path.mkdir(parents=True, exist_ok=True)
    for name in molecules:
        parser(name)
    visualize_molecules()
    #paths = ["1kx5.csv", "4V6X.csv", "6WHT.csv", "6tar.csv"]
    #for p in paths:
    #    print(f"Number of overlaps: {check_overlaps(path / p)}")

    