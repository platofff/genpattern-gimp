#include "convex_area.h"

#include <math.h>

gp_float gp_convex_area(GPPolygon* polygon) {
  gp_float p = polygon->y_ptr[0] * (polygon->x_ptr[polygon->size - 1] - polygon->x_ptr[1]) +
            polygon->y_ptr[polygon->size - 1] * (polygon->x_ptr[polygon->size - 2] - polygon->x_ptr[0]);
  for (int32_t i = 0; i < polygon->size - 2; i++) {
    p += polygon->y_ptr[i + 1] * (polygon->x_ptr[i] - polygon->x_ptr[i + 2]);
  }
  return gp_fabs(p) * (gp_float)0.5;
}