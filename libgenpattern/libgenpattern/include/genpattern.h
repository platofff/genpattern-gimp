#ifndef _GP_GENPATTERN_H
#define _GP_GENPATTERN_H 1

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>

#include "convex_hull.h"
#include "polygon.h"

typedef struct {
  GPPolygon* polygons;
  GPImgAlpha* alphas;
  int32_t* collection_ids;
  uint8_t t;
  size_t work_size;
  size_t* next_work;
  pthread_mutex_t next_work_mtx;
} GPCPParams;

typedef struct {
  GPCPParams* params;
  size_t thread_id;
} GPCPThreadData;

int gp_genpattern(GPImgAlpha* alphas,
                  int32_t* collections_sizes,
                  int32_t collections_len,
                  int32_t canvas_width,
                  int32_t canvas_height,
                  uint8_t t,
                  int32_t out_len,
                  GPVector* out);

#endif