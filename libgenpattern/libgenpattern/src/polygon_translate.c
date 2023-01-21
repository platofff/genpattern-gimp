#include "polygon_translate.h"

void gp_polygon_translate(GPPolygon* polygon, float x_off_val, float y_off_val) {
  for (int32_t i = 0; i < polygon->size; i++) {
    polygon->x_ptr[i] += x_off_val;
    polygon->y_ptr[i] += y_off_val;
  }
  polygon->bounds.xmax += x_off_val;
  polygon->bounds.xmin += x_off_val;
  polygon->bounds.ymax += y_off_val;
  polygon->bounds.ymin += y_off_val;
}
