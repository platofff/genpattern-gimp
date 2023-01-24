#ifndef _GP_SUITABILITY_H
#define _GP_SUITABILITY_H 1
#include "box.h"
#include "polygon.h"

float gp_suitability(GPPolygon* polygon, GPPolygon* polygons, int32_t polygons_len, GPPolygon* canvas);

#endif
