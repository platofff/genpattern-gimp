#ifndef _GP_GENPATTERN_H
#define _GP_GENPATTERN_H 1

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>

#include "convex_hull.h"
#include "exports.h"
#include "polygon.h"

typedef union {
  struct {
    GPPolygon* polygons;
    size_t work_size;
    GPPolygon canvas_polygon;
    pthread_mutex_t next_work_mtx;
    size_t* next_work;
    int32_t* max_size;
    //
    GPPolygon* out_polygons;
    size_t *out_polygons_len;
    GPPolygon* polygon_buffers;
    GPPolygon* collection;
    int32_t collection_len;
    GPVector* grid;
    size_t current;
    gp_float target;
    gp_float initial_step;
    GPPolygon* canvas_outside_areas;
  } gp;
  struct {
    GPPolygon* polygons;
    size_t work_size;
    GPPolygon canvas_polygon;
    pthread_mutex_t next_work_mtx;
    size_t* next_work;
    int32_t* max_size;
    //
    GPImgAlpha* alphas;
    uint8_t t;
  } cp;
} GPParams;

typedef struct {
  GPParams* params;
  size_t thread_id;
} GPThreadData;

typedef struct {
  GPVector offset;
  int32_t collection_idx;
  int32_t idx;
} GPResult;

LIBGENPATTERN_API int gp_init(void);

LIBGENPATTERN_API int gp_genpattern(GPImgAlpha* alphas,
                                    int32_t* collections_sizes,
                                    int32_t collections_len,
                                    int32_t canvas_width,
                                    int32_t canvas_height,
                                    uint8_t t,
                                    int32_t min_dist,
                                    int32_t grid_resolution,
                                    uint32_t initial_step,
                                    size_t threads_num,
                                    GPResult** out,
                                    size_t* out_len);

#endif