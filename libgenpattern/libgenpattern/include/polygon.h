#ifndef _GP_POLYGON_H
#define _GP_POLYGON_H 1

#include <stdint.h>

#include "basic_geometry.h"
#include "box.h"

typedef struct {
  float *x_ptr, *y_ptr;
  int32_t size;
  GPBox bounds;
  GPVector base_offset;
  float area;
} GPPolygon;

void gp_polygon_free(GPPolygon* p);
void gp_polygon_copy(GPPolygon* dst, GPPolygon* src);
void gp_box_to_polygon(GPBox* box, GPPolygon* polygon);

#define GP_POLYGON_POINT(polygon, idx) \
  (GPPoint) { polygon->x_ptr[idx], polygon->y_ptr[idx] }
#define GP_PREV_IDX(polygon, idx) idx == 0 ? polygon->size - 1 : idx - 1
#define GP_NEXT_IDX(polygon, idx) (idx + 1) % polygon->size

#endif