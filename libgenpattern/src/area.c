#include "area.h"

#include <math.h>

float polygon_area(Polygon *polygon) {
  float p = 0.0f;
  p += polygon->y_ptr[0] *
       (polygon->x_ptr[polygon->size - 1] - polygon->x_ptr[1]);
  p += polygon->y_ptr[polygon->size - 1] *
       (polygon->x_ptr[polygon->size - 2] - polygon->x_ptr[0]);
  for (int32_t i = 0; i < polygon->size - 2; i++) {
    p += polygon->y_ptr[i + 1] * (polygon->x_ptr[i] - polygon->x_ptr[i + 2]);
  }
  return fabsf(p) * 0.5f;
}