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

void gp_box_to_polygon(GPBox* box, GPPolygon* polygon) {
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
  polygon->collection_id = -1;
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

void gp_polygon_copy(GPPolygon* dst, GPPolygon* src) {
  memcpy(dst, src, sizeof(GPPolygon));
  
  size_t fsize = sizeof(float) * dst->size;
  dst->x_ptr = NULL;
  dst->x_ptr = aligned_alloc(32, fsize);
  GP_CHECK_ALLOC(dst->x_ptr);
  dst->y_ptr = NULL;
  dst->y_ptr = aligned_alloc(32, fsize);
  GP_CHECK_ALLOC(dst->y_ptr);

  // meaningless in my case
  //memcpy(dst->x_ptr, src->x_ptr, fsize);
  //memcpy(dst->y_ptr, src->y_ptr, fsize);
}