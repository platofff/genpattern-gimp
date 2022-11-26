#include "translate.h"

void polygon_translate(Polygon *polygon, float x_off_val, float y_off_val) {
  for (int32_t i = 0; i < polygon->size; i++) {
    polygon->x_ptr[i] += x_off_val;
    polygon->y_ptr[i] += y_off_val;
  }
}
