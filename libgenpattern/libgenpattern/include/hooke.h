#ifndef _GP_HOOKE_H
#define _GP_HOOKE_H 1

#include "basic_geometry.h"
#include "suitability.h"

#define GP_HOOKE_ACCURACY 2  // TODO
#define GP_HOOKE_CACHE_SIZE 3

gp_float gp_maximize_suitability(GPPoint b1,
                              gp_float step,
                              gp_float target,
                              GPSParams* sp,
                              GPPoint* out,
                              int32_t* out_len,
                              GPPolygon** out_polygons);

typedef struct {
  GPVector args[GP_HOOKE_CACHE_SIZE];
  gp_float results[GP_HOOKE_CACHE_SIZE];
  GPPolygon* out[GP_HOOKE_CACHE_SIZE];
  int32_t out_len[GP_HOOKE_CACHE_SIZE];
  int32_t next;
  bool full;
} GPSCache;

#endif