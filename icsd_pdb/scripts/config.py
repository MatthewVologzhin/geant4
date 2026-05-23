import pathlib

# Корень проекта
BASE_DIR = pathlib.Path(__file__).resolve().parent.parent

# Ван-дер-Ваальсовы радиусы по Сантьяго Альваресу (Dalton Trans., 2013, 42, 8617)
# Значения взяты из Таблицы 1 статьи.
radius_map = {
    "H": 1.20,
    "C": 1.77,
    "N": 1.66,
    "O": 1.50,
    "F": 1.46,
    "NA": 2.50,
    "MG": 2.51,  # Magnesium
    "P": 1.90,
    "S": 1.89,
    "CL": 1.82,  # Chlorine
    "K": 2.73,
    "CA": 2.62,
    "MN": 2.45,  # Manganese
    "FE": 2.44,
    "CO": 2.40,
    "NI": 2.40,
    "CU": 2.38,
    "ZN": 2.39,
    "empty": 1.0
}

# Цветовая схема (общепринятая CPK + расширения)
color_map = {
    "H": "#FFFFFF",      # Белый
    "C": "#909090",      # Серый (лучше, чем черный, на темном фоне Plotly)
    "N": "#3050F8",      # Насыщенный синий
    "O": "#FF0D0D",      # Красный
    "P": "#FF8000",      # Оранжевый
    "S": "#FFFF30",      # Желтый
    "CL": "#1FF01F",     # Ярко-зеленый
    "MG": "#8AFF00",     # Светло-зеленый
    "MN": "#9C7AC7",     # Пурпурный
    "ZN": "#7D80B0",     # Темно-сине-серый
    "FE": "#E06633",     # Ржаво-оранжевый
    "CA": "#808090",     # Кальций (часто встречается в PDB)
    "NA": "#0000FF",     # Натрий
    "K":  "#8F40D4",     # Калий
    "empty": "#2F2F2F"   # Серый фон
}

molecules = {
    "histone": {
        "chain": {"I", "J"},
        "cif": BASE_DIR / "input" / "1kx5.cif",
        "csv": BASE_DIR / "results" / "data" / "1kx5.csv",
        "skip": 1
    },
    "NMDA": {
        # Для NMDA-рецептора (6WHT) цепи обычно A, B, C, D (гетеротетрамер)
        "chain": {}, 
        "cif": BASE_DIR / "input" / "6WHT.cif",
        "csv": BASE_DIR / "results" / "data" / "6WHT.csv",
        "skip": 5
    },
    "ribosome": {
        # Рибосома (4V6X) очень большая, укажем основные цепи или оставим пустым для "all"
        "chain": {}, 
        "cif": BASE_DIR / "input" / "4V6X.cif",
        "csv": BASE_DIR / "results" / "data" / "4V6X.csv",
        "skip": 25 # Увеличил пропуск для производительности
    },
    "cytoskeleton": {
        "chain": {},
        "cif": BASE_DIR / "input" / "6tar.cif",
        "csv": BASE_DIR / "results" / "data" / "6tar.csv",
        "skip": 10
    },
}