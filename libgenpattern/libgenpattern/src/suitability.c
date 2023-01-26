#include <math.h>

#include "convex_intersection_area.h"
#include "suitability.h"
#include "convex_distance.h"
#include "misc.h"

static inline float _gp_borders_suitability(GPPolygon* polygon, GPPolygon* canvas, bool* intersected) {
  float dist = -gp_boxes_inner_distance(canvas->bounds, polygon->bounds);
  if (dist <= 0) {
    return dist;
  }
  return polygon->area -
         gp_convex_intersection_area(canvas, polygon, intersected);  // TODO: do it without boxes intersection check
}

float gp_suitability(GPPolygon* polygon,
                     float target,
                     GPPolygon* polygons,
                     int32_t polygons_len,
                     GPPolygon* collection,
                     int32_t collection_len,
                     GPPolygon* canvas) {
  bool intersected = false;
  float res = _gp_borders_suitability(polygon, canvas, &intersected);

  for (int32_t i = 0; i < polygons_len; i++) {
    bool _intersected;
    float intersection = gp_convex_intersection_area(polygon, &polygons[i], &_intersected);
    if (_intersected) {
      if (!intersected) {
        intersected = true;
        res = 0.f;
      }
      res += intersection;
    }
  }

  if (intersected) {
    return res;
  }

  res = MAX(res, -target);
  for (int32_t i = 0; i < collection_len; i++) {
    float dist = -gp_convex_distance(polygon, &collection[i]);
    res = MAX(res, dist);
  }

  return res;
}