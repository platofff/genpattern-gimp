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
void gp_polygon_copy(GPPolygon* dst, const GPPolygon* src);
void gp_polygon_copy_all(GPPolygon* dst, const GPPolygon* src);
int gp_box_to_polygon(GPBox* box, GPPolygon* polygon);
void gp_polygon_add_point(GPPolygon* polygon, GPPoint point);
int gp_polygon_init_empty(GPPolygon* res, int32_t max_size);
int gp_canvas_outside_areas(float xres, float yres, GPPolygon* polygons);

#define GP_POLYGON_POINT(polygon, idx) \
  (GPPoint) { polygon->x_ptr[idx], polygon->y_ptr[idx] }
#define GP_PREV_IDX(polygon, idx) idx == 0 ? polygon->size - 1 : idx - 1
#define GP_NEXT_IDX(polygon, idx) (idx + 1) % polygon->size

#define GP_AREA_LEFT 0
#define GP_AREA_TOP 1
#define GP_AREA_BOTTOM 2
#define GP_AREA_RIGHT 3
#define GP_AREA_TOP_LEFT 4
#define GP_AREA_TOP_RIGHT 5
#define GP_AREA_BOTTOM_RIGHT 6
#define GP_AREA_BOTTOM_LEFT 7

#endif