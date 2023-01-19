#ifndef _GP_GENPATTERN_H
#define _GP_GENPATTERN_H 1

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>

#include "polygon.h"
#include "convex_hull.h"

typedef struct {
  GPPolygon* polygons;
  GPImgAlpha** alpha_ptrs;
  uint8_t t;
  uint64_t* next_work;
  pthread_mutex_t next_work_mtx;
} GPCPParams;

typedef struct {
  GPCPParams* params;
  size_t thread_id;
} GPCPThreadData;

#endif