#ifndef PHYSICS_HPP
#define PHYSICS_HPP

#include "util/common.hpp"
#include "util/math_util.hpp"
#include "util/template.hpp"

using ObjectId = u32;

#define OVERFLOW_CELL_INDEX_SENTINEL -1
#define CELL_CAPACITY 8
struct GridCell {
    ObjectId objects[CELL_CAPACITY];
    int count;
    int overflow_cell;
};

struct SpatialGrid {
    GridCell* cells;
    int dimension_x;
    int dimension_y;
    float cell_size;
    DArray<GridCell> overflow_cells;

    void initialize(int dim_x, int dim_y, float p_cell_size);
    void cleanup();
    void clear_entries();
    int size();
    void add(cobot::vec2 position, ObjectId object);
    void remove(cobot::vec2 position, int object);
    GridCell* get_cell(cobot::vec2 position);
    int calculate_cell_index(cobot::vec2 position);
private:

    void add_to_cell(GridCell& cell, ObjectId object);
    void remove_from_cell(GridCell& cell, ObjectId object);
};

#endif // PHYSICS_HPP
