#ifndef _GP_GENPATTERN_H
#define _GP_GENPATTERN_H 1

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>

#include "convex_hull.h"
#include "polygon.h"

typedef union {
  struct {
    GPPolygon* polygons;
    size_t work_size;
    GPPolygon canvas_polygon;
    pthread_mutex_t next_work_mtx;
    size_t* next_work;
    //
    GPPolygon* polygon_buffers;
    GPPolygon* collection;
    int32_t collection_len;
    GPVector* grid;
    size_t current;
  } gp;
  struct {
    GPPolygon* polygons;
    size_t work_size;
    GPPolygon canvas_polygon;
    pthread_mutex_t next_work_mtx;
    size_t* next_work;
    //
    GPImgAlpha* alphas;
    uint8_t t;
    int32_t* max_size;
  } cp;
} GPParams;

typedef struct {
  GPParams* params;
  size_t thread_id;
} GPThreadData;

int gp_genpattern(GPImgAlpha* alphas,
                  int32_t* collections_sizes,
                  int32_t collections_len,
                  int32_t canvas_width,
                  int32_t canvas_height,
                  uint8_t t,
                  int32_t grid_resolution,
                  int32_t out_len,
                  GPVector* out);

#endif