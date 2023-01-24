#include "polygon_translate.h"

void gp_polygon_translate(GPPolygon* dst_polygon, GPPolygon* src_polygon, float x_off_val, float y_off_val) {
  for (int32_t i = 0; i < src_polygon->size; i++) {
    dst_polygon->x_ptr[i] = src_polygon->x_ptr[i] + x_off_val;
    dst_polygon->y_ptr[i] = src_polygon->y_ptr[i] + y_off_val;
  }
  dst_polygon->bounds.xmax = src_polygon->bounds.xmax + x_off_val;
  dst_polygon->bounds.xmin = src_polygon->bounds.xmin + x_off_val;
  dst_polygon->bounds.ymax = src_polygon->bounds.ymax + y_off_val;
  dst_polygon->bounds.ymin = src_polygon->bounds.ymin + y_off_val;
}
