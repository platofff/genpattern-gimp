#ifndef _GP_SUITABILITY_H
#define _GP_SUITABILITY_H 1

#include "box.h"
#include "polygon.h"

typedef struct {
  GPPolygon* polygons_buffer;
  gp_float target;
  GPPolygon* polygons;
  size_t polygons_len;
  GPPolygon* collection;
  size_t collection_len;
  GPPolygon* canvas;
  GPPolygon* canvas_outside_areas;
  GPPolygon* ref;
} GPSParams;

gp_float gp_suitability(GPSParams p, GPPolygon* polygons_buffer, GPPolygon** out, int32_t* out_len);

#endif
