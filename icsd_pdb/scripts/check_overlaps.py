import pathlib
import pandas as pd
import numpy as np
from scipy.spatial import cKDTree
# Предполагаем, что radius_map импортируется отсюда
from config import radius_map 

def check_overlaps(csv_path):
    csv = pathlib.Path(csv_path)
    if not csv.exists():
        print(f"File {csv} doesn't exist!")
        return None

    # Читаем CSV
    df = pd.read_csv(csv)
    n = len(df)
    if n == 0:
        return 0

    # Рекомендуется использовать явные имена колонок вместо df.iloc
    # Например, если колонки называются 'type', 'x', 'y', 'z':
    try:
        type_column = df.columns[0]  # или явно 'type'
        coord_columns = df.columns[1:4]  # или явно ['x', 'y', 'z']
        
        # Альтернативный безопасный вариант (раскомментируйте, если знаете имена):
        # type_column = 'type'
        # coord_columns = ['x', 'y', 'z']
        
        coords = df[coord_columns].to_numpy()
    except KeyError as e:
        print(f"Ошибка при чтении колонок: {e}")
        return None

    # Маппинг радиусов с проверкой на пропуски
    radii_series = df[type_column].map(radius_map)
    
    # Проверяем, не появились ли NaN после маппинга
    if radii_series.isna().any():
        unmapped_keys = df[type_column][radii_series.isna()].unique()
        print(f"Внимание: в radius_map отсутствуют ключи: {unmapped_keys}")
        # Можно либо заполнить дефолтным значением, либо отфильтровать строки:
        # radii_series = radii_series.fillna(0) 
        
    radii = radii_series.to_numpy()

    # Проверка на корректность типов данных (должны быть float)
    if not np.issubdtype(coords.dtype, np.number):
        print("Ошибка: Координаты содержат нечисловые значения.")
        return None

    # Строим дерево
    # Если у вас периодические граничные условия, укажите boxsize, например:
    # tree = cKDTree(coords, boxsize=[L, L, L])
    tree = cKDTree(coords)

    max_radius = np.nanmax(radii)  # nanmax устойчив к случайным NaN
    max_search_distance = 2 * max_radius

    # Поиск кандидатов
    candidate_pairs = tree.query_pairs(r=max_search_distance)

    if not candidate_pairs:
        print(f"Total particles: {n}, Overlaps: 0")
        return 0

    pairs = np.array(list(candidate_pairs))
    idx_i = pairs[:, 0]
    idx_j = pairs[:, 1]

    # Считаем расстояния
    deltas = coords[idx_i] - coords[idx_j]
    
    # Если использовался boxsize в cKDTree, здесь тоже нужно учесть периодичность.
    # Для этого в scipy.spatial есть встроенные механизмы, но если boxsize нет, оставляем так:
    deltas_square = np.sum(deltas ** 2, axis=-1)

    # Проверка на пересечение
    radii_sum_square = (radii[idx_i] + radii[idx_j]) ** 2
    overlaps = deltas_square < radii_sum_square  # строгое неравенство обычно предпочтительнее для касания

    overlaps_number = np.sum(overlaps)
    print(f"Total particles: {n}, Overlapping pairs: {overlaps_number}")
    intersecting_particle_indices = np.unique(pairs[overlaps])
    num_intersecting_particles = len(intersecting_particle_indices)

    print(f"Всего частиц: {n}")
    print(f"Уникальных пересекающихся частиц: {num_intersecting_particles} ({(num_intersecting_particles/n)*100:.1f}%)")
    print(f"Всего связей-пересечений (пар): {overlaps_number}")
    return overlaps_number