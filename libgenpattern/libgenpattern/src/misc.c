#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "misc.h"

static inline int32_t _gp_lrand(void) {
  int32_t r = rand() & 0x7FFF;
  r = (r << 15) | (rand() & 0x7FFF);
  return r | ((rand() & 1) << 30);
}

void gp_array_shuffle(void* _arr, size_t sz, int32_t len) {
  int8_t* arr = (int8_t*)_arr;
  int8_t* tmp = malloc(sz);
  GP_CHECK_ALLOC(tmp);
  for (int32_t i = len - 1; i > 0; i--) {
    int32_t j = _gp_lrand() % (i + 1);
    memcpy(tmp, &arr[i * sz], sz);
    memcpy(&arr[i * sz], &arr[j * sz], sz);
    memcpy(&arr[j * sz], tmp, sz);
  }
  free(tmp);
}

void gp_grid_init(float x, float y, float resolution, GPVector** grid, size_t* len) {
  size_t n = x / resolution;
  size_t m = y / resolution;
  *len = m * n;
  *grid = malloc(*len * sizeof(GPVector));
  GP_CHECK_ALLOC(*grid);
  for (size_t i = 0; i < n; i++) {
    for (size_t j = 0; j < m; j++) {
      (*grid)[m * i + j].x = i * resolution;
      (*grid)[m * i + j].y = j * resolution;
    }
  }
}