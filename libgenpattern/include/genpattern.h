#include <stdint.h>
#include <stdlib.h>
#include <threads.h>

#include "basic_geometry.h"
#include "convex_hull.h"

typedef struct {
  Polygon **polygon_ptrs;
  ImgAlpha **alpha_ptrs;
  uint8_t t;
  uint64_t *next_work;
  mtx_t next_work_mtx;
} CPParams;

typedef struct {
  CPParams *params;
  size_t thread_id;
} CPThreadData;
