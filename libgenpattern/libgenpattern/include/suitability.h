#ifndef _GP_SUITABILITY_H
#define _GP_SUITABILITY_H 1

#include "box.h"
#include "polygon.h"

typedef struct {
  GPPolygon* polygon;
  float target;
  GPPolygon* polygons;
  int32_t polygons_len;
  GPPolygon* collection;
  int32_t collection_len;
  GPPolygon* canvas;
  GPPolygon* ref;
} GPSParams;

float gp_suitability(GPSParams p);

#endif