#pragma once

#include "nanoflann.hpp"
#include "G4ThreeVector.hh"
#include "Randomize.hh"
#include <vector>

// 1. Простой адаптер: учим nanoflann читать обычный std::vector<G4ThreeVector>
struct CellCloud {
    std::vector<G4ThreeVector> pts;

    // nanoflann требует ровно 3 метода:
    inline size_t kdtree_get_point_count() const { return pts.size(); }
    
    // G4ThreeVector поддерживает доступ по индексу [dim]: 0=x, 1=y, 2=z
    inline double kdtree_get_pt(size_t idx, size_t dim) const { return pts[idx][dim]; }

    template <class BBOX> 
    bool kdtree_get_bbox(BBOX&) const { return false; }
};

// 2. Функция генерации точек
class CellPacker {
public:
    static std::vector<G4ThreeVector> GeneratePositions(
        const G4ThreeVector& boxSize, 
        double cellRadius, 
        size_t targetN,
        size_t maxFails = 2000)
    {
        CellCloud cloud;
        cloud.pts.reserve(targetN); // Сразу резервируем память

        // Тип динамического дерева (L2_Simple = обычное евклидово расстояние)
        using KDTree = nanoflann::KDTreeSingleIndexDynamicAdaptor<
            nanoflann::L2_Simple_Adaptor<double, CellCloud>,
            CellCloud,
            3 /* 3D */
        >;

        // Создаем дерево (3D, ссылка на наше облако, макс. размер листа = 10)
        KDTree tree(3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10));

        // ВНИМАНИЕ: nanoflann сравнивает квадраты расстояний!
        const double minDist = 2.0 * cellRadius;
        const double minDistSq = minDist * minDist;

        // Пределы генерации центров клеток (чтобы клетка не торчала наружу среды)
        const double xHalf = boxSize.x() * 0.5 - cellRadius;
        const double yHalf = boxSize.y() * 0.5 - cellRadius;
        const double zHalf = boxSize.z() * 0.5 - cellRadius;

        if (xHalf <= 0.0 || yHalf <= 0.0 || zHalf <= 0.0) return {};

        size_t fails = 0;
        // В этот вектор nanoflann складывает найденные пересечения
        std::vector<nanoflann::ResultItem<size_t, double>> matches;

        while (cloud.pts.size() < targetN && fails < maxFails) {
            // 1. Случайная точка-кандидат
            const double candidate[3] = {
                (G4UniformRand() * 2.0 - 1.0) * xHalf,
                (G4UniformRand() * 2.0 - 1.0) * yHalf,
                (G4UniformRand() * 2.0 - 1.0) * zHalf
            };

            // 2. Ищем в дереве соседей ближе, чем 2*R
            matches.clear();
            nanoflann::RadiusResultSet<double, size_t> resultSet(minDistSq, matches);
            tree.findNeighbors(resultSet, candidate);

            // 3. Если соседей нет (matches пуст) — пересечений нет!
            if (matches.empty()) {
                cloud.pts.emplace_back(candidate[0], candidate[1], candidate[2]);
                
                // Добавляем индекс последней добавленной точки в дерево:
                const size_t newIdx = cloud.pts.size() - 1;
                tree.addPoints(newIdx, newIdx);

                fails = 0; // Сбрасываем счетчик неудач
            } else {
                ++fails;
            }
        }

        // Возвращаем готовый вектор клеток
        return cloud.pts;
    }
};