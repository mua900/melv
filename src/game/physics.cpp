#include "physics.hpp"

int spatial_hash(cobot::vec2 pos);
int cell_hash(cobot::vec2 pos, float cell_size);

void SpatialGrid::initialize(int dim_x, int dim_y, float p_cell_size)
{
    dimension_x = dim_x;
    dimension_y = dim_y;
    cell_size = p_cell_size;

    cells = new GridCell[dim_x * dim_y];
}

void SpatialGrid::cleanup()
{
    if (cells)
        delete[] cells;
}

void SpatialGrid::clear_entries()
{
    int size = this->size();
    for (int i = 0; i < size; i++)
    {
        cells[i].count = 0;
        cells[i].overflow_cell = 0;
    }

    overflow_cells.discard_data();
}

int SpatialGrid::size()
{
    return dimension_x * dimension_y;
}

int SpatialGrid::calculate_cell_index(cobot::vec2 position)
{
    int hash = cell_hash(position, cell_size);
    hash = abs(hash);
    hash %= size();
    return hash;
}

void SpatialGrid::add(cobot::vec2 position, ObjectId object)
{
    int cell_index = calculate_cell_index(position);
    add_to_cell(cells[cell_index], object);
}

void SpatialGrid::remove(cobot::vec2 position, int object)
{
    int cell_index = calculate_cell_index(position);
    remove_from_cell(cells[cell_index], object);
}

GridCell* SpatialGrid::get_cell(cobot::vec2 position)
{
    int cell_index = calculate_cell_index(position);
    return &cells[cell_index];
}

// if we want the entries to be unique we would need to check all of them which we can do in a separate function as an opt in way
void SpatialGrid::add_to_cell(GridCell& cell, ObjectId object)
{
    if (cell.count == CELL_CAPACITY)
    {
        if (cell.overflow_cell == OVERFLOW_CELL_INDEX_SENTINEL)
        {
            int oc = overflow_cells.add(GridCell());
            cell.overflow_cell = oc;
        }

        add_to_cell(overflow_cells.get_ref(cell.overflow_cell), object);
        return;
    }

    cell.objects[cell.count] = object;
    cell.count += 1;
}

void SpatialGrid::remove_from_cell(GridCell& cell, ObjectId object)
{
    for (int i = 0; i < cell.count; i++)
    {
        if (cell.objects[i] == object)
        {
            cell.objects[i] = cell.objects[cell.count - 1];
            cell.count -= 1;
            return;
        }
    }

    if (cell.overflow_cell != OVERFLOW_CELL_INDEX_SENTINEL)
    {
        remove_from_cell(overflow_cells.get_ref(cell.overflow_cell), object);
    }
}


int spatial_hash(cobot::vec2 pos)
{
    return int(floor(pos.x) * 962623) ^ int(floor(pos.y) * 1193771);
}

int cell_hash(cobot::vec2 pos, float cell_size)
{
    return spatial_hash(pos / cell_size);
}
