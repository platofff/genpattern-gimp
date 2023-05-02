#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "convex_area.h"
#include "polygon.h"

void gp_polygon_free(GPPolygon* p) {
  gp_aligned_free(p->x_ptr);
  gp_aligned_free(p->y_ptr);
}

int gp_box_to_polygon(GPBox* box, GPPolygon* polygon) {
  polygon->x_ptr = gp_aligned_alloc(64, 4 * sizeof(gp_float));
  GP_CHECK_ALLOC(polygon->x_ptr);
  polygon->y_ptr = gp_aligned_alloc(64, 4 * sizeof(gp_float));
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
  polygon->base_offset.x = 0;
  polygon->base_offset.y = 0;
  polygon->area = gp_convex_area(polygon);
  return 0;
}

/*
void gp_canvas_outside_area(gp_float xres, gp_float yres, GPPolygon* polygons) {
  gp_float mval = GP_MAX(xres, yres);
  GPBox box1 = {.xmin = -mval, .ymin = -mval, .xmax = 0, .ymax = yres + mval};
  GPBox box2 = {.xmin = 0, .ymin = -mval, .xmax = xres, .ymax = 0};
  GPBox box3 = {.xmin = 0, .ymin = yres, .xmax = xres, .ymax = yres + mval};
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
  memcpy(dst->x_ptr, src->x_ptr, src->size * sizeof(gp_float));
  memcpy(dst->y_ptr, src->y_ptr, src->size * sizeof(gp_float));
}

static inline bool _gp_are_collinear(const GPPoint a, const GPPoint b, const GPPoint c) {
  return gp_fabs((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x)) < GP_EPSILON;
}

// Doesn't compute area
// Doesn't check array bounds
void gp_polygon_add_point(GPPolygon* polygon, const GPPoint point) {
  // ???
  for (int32_t i = 0; i < polygon->size; i++) {
    if (gp_fabs(polygon->x_ptr[i] - point.x) < GP_EPSILON &&
        gp_fabs(polygon->y_ptr[i] - point.y) < GP_EPSILON) {
      return;
    }
  }

  polygon->bounds.xmin = GP_MIN(polygon->bounds.xmin, point.x);
  polygon->bounds.ymin = GP_MIN(polygon->bounds.ymin, point.y);
  polygon->bounds.xmax = GP_MAX(polygon->bounds.xmax, point.x);
  polygon->bounds.ymax = GP_MAX(polygon->bounds.ymax, point.y);

  if (polygon->size >= 2) {
    GPPoint prev = GP_POLYGON_POINT(polygon, polygon->size - 1);
    GPPoint prev_prev = GP_POLYGON_POINT(polygon, polygon->size - 2);

    if (_gp_are_collinear(prev_prev, prev, point)) {
      polygon->x_ptr[polygon->size - 1] = point.x;
      polygon->y_ptr[polygon->size - 1] = point.y;
      return;
    }
  }

  polygon->x_ptr[polygon->size] = point.x;
  polygon->y_ptr[polygon->size] = point.y;
  polygon->size++;
}

int gp_polygon_init_empty(GPPolygon* res, int32_t max_size) {
  res->size = 0;
  res->x_ptr = gp_aligned_alloc(64, sizeof(gp_float) * max_size);
  GP_CHECK_ALLOC(res->x_ptr);
  res->y_ptr = gp_aligned_alloc(64, sizeof(gp_float) * max_size);
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
int gp_canvas_outside_areas(gp_float xres, gp_float yres, GPPolygon* polygons) {
  gp_float mval = GP_MAX(xres, yres);
  GPBox boxes[8] = {{-mval, 0, 0, yres},           {0, yres, xres, yres + mval},
                    {0, -mval, xres, 0},           {xres, 0, mval + xres, yres},
                    {-mval, yres, 0, yres + mval}, {xres, yres, xres + mval, yres + mval},
                    {xres, -mval, xres + mval, 0}, {-mval, -mval, 0, 0}};

  for (int32_t i = 0; i < 8; i++) {
    int res = gp_box_to_polygon(&boxes[i], &polygons[i]);
    if (res != 0) {
      return res;
    }
  }
  return 0;
}
