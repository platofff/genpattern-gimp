#include "debug.h"

void gp_debug_polygons(GPPolygon* polygons, int32_t len) {
  printf("(");
  for (int32_t i = 0; i < len; i++) {
    printf("(");
    for (int32_t v = 0; v < polygons[i].size; v++) {
      printf("(%f,%f),", polygons[i].x_ptr[v], polygons[i].y_ptr[v]);
    }
    puts("),");
  }
  puts(")");
}

