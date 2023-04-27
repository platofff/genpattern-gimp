#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "convex_area.h"
#include "misc.h"
#include "polygon.h"

void gp_polygon_free(GPPolygon* p) {
  _aligned_free(p->x_ptr);
  _aligned_free(p->y_ptr);
}

int gp_box_to_polygon(GPBox* box, GPPolygon* polygon) {
  polygon->x_ptr = aligned_alloc(32, 4 * sizeof(float));
  GP_CHECK_ALLOC(polygon->x_ptr);
  polygon->y_ptr = aligned_alloc(32, 4 * sizeof(float));
  GP_CHECK_ALLOC(polygon->y_ptr);
  polygon->size = 4;

  polygon->x_ptr[0] = box->xmin;
  polygon->x_ptr[1] = box->xmin;
  polygon->x_ptr[2] = box->xmax;
  polygon->x_ptr[3] = box->xmax;
  polygon->y_ptr[0] = box->ymax;
  polygon->y_ptr[1] = box->ymin;
  polygon->y_ptr[2] = box->ymin;
  polygon->y_ptr[3] = box->ymax;

  memcpy(&polygon->bounds, box, sizeof(GPBox));
  polygon->base_offset.x = 0.f;
  polygon->base_offset.y = 0.f;
  polygon->area = gp_convex_area(polygon);
  return 0;
}

/*
void gp_canvas_outside_area(float xres, float yres, GPPolygon* polygons) {
  float mval = MAX(xres, yres);
  GPBox box1 = {.xmin = -mval, .ymin = -mval, .xmax = 0.f, .ymax = yres + mval};
  GPBox box2 = {.xmin = 0.f, .ymin = -mval, .xmax = xres, .ymax = 0.f};
  GPBox box3 = {.xmin = 0.f, .ymin = yres, .xmax = xres, .ymax = yres + mval};
  GPBox box4 = {.xmin = xres, .ymin = -mval, .xmax = xres + mval, .ymax = yres + mval};
  gp_box_to_polygon(&box1, &polygons[0]);
  gp_box_to_polygon(&box2, &polygons[1]);
  gp_box_to_polygon(&box3, &polygons[2]);
  gp_box_to_polygon(&box4, &polygons[3]);
}
*/

void gp_polygon_copy(GPPolygon* dst, const GPPolygon* src) {
  dst->area = src->area;
  dst->base_offset.x = src->base_offset.x;
  dst->base_offset.y = src->base_offset.y;
  dst->bounds.xmax = src->bounds.xmax;
  dst->bounds.xmin = src->bounds.xmin;
  dst->bounds.ymax = src->bounds.ymax;
  dst->bounds.ymin = src->bounds.ymin;
  dst->size = src->size;
}

void gp_polygon_copy_all(GPPolygon* dst, const GPPolygon* src) {
  gp_polygon_copy(dst, src);
  memcpy(dst->x_ptr, src->x_ptr, src->size * sizeof(float));
  memcpy(dst->y_ptr, src->y_ptr, src->size * sizeof(float));
}

// Doesn't compute area
// Doesn't check array bounds
void gp_polygon_add_point(GPPolygon* polygon, GPPoint point) {
  polygon->bounds.xmin = MIN(polygon->bounds.xmin, point.x);
  polygon->bounds.ymin = MIN(polygon->bounds.ymin, point.y);
  polygon->bounds.xmax = MAX(polygon->bounds.xmax, point.x);
  polygon->bounds.ymax = MAX(polygon->bounds.ymax, point.y);
  polygon->x_ptr[polygon->size] = point.x;
  polygon->y_ptr[polygon->size] = point.y;
  polygon->size++;
}

int gp_polygon_init_empty(GPPolygon* res, int32_t max_size) {
  res->size = 0;
  res->x_ptr = aligned_alloc(32, sizeof(float) * max_size);
  GP_CHECK_ALLOC(res->x_ptr);
  res->y_ptr = aligned_alloc(32, sizeof(float) * max_size);
  GP_CHECK_ALLOC(res->y_ptr);
  res->bounds.xmin = INFINITY;
  res->bounds.ymin = INFINITY;
  res->bounds.xmax = -INFINITY;
  res->bounds.ymax = -INFINITY;
  res->base_offset.x = 0;
  res->base_offset.y = 0;
  return 0;
}

// polygons buffer size must be at least 8
int gp_canvas_outside_areas(float xres, float yres, GPPolygon* polygons) {
  GPBox boxes[8] = {{.xmin = -INFINITY, .ymin = 0.f, .xmax = 0.f, .ymax = yres},
                    {.xmin = -INFINITY, .ymin = -INFINITY, .xmax = xres, .ymax = 0.f},
                    {.xmin = 0.f, .ymin = yres, .xmax = xres, .ymax = INFINITY},
                    {.xmin = xres, .ymin = 0.f, .xmax = INFINITY, .ymax = yres},
                    // Corner boxes
                    {.xmin = -INFINITY, .ymin = -INFINITY, .xmax = 0.f, .ymax = 0.f},
                    {.xmin = xres, .ymin = -INFINITY, .xmax = INFINITY, .ymax = 0.f},
                    {.xmin = xres, .ymin = yres, .xmax = INFINITY, .ymax = INFINITY},
                    {.xmin = -INFINITY, .ymin = yres, .xmax = 0.f, .ymax = INFINITY}};

  for (int32_t i = 0; i < 8; i++) {
    int res = gp_box_to_polygon(&boxes[i], &polygons[i]);
    if (res != 0) {
      return res;
    }
  }
  return 0;
}
