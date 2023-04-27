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
  float intersection_area;
  gp_convex_intersection_area(canvas, polygon, intersected, &intersection_area, NULL);
  return polygon->area - intersection_area;
}

float gp_suitability(GPSParams p) {
  bool intersected = false;
  float res = _gp_borders_suitability(p.polygon, p.canvas, &intersected);

  for (int32_t i = 0; i < p.polygons_len; i++) {
    bool _intersected;
    float intersection_area;
    gp_convex_intersection_area(p.polygon, &p.polygons[i], &_intersected, &intersection_area, NULL);
    if (_intersected) {
      if (!intersected) {
        intersected = true;
        res = 0.f;
      }
      res += intersection_area;
    }
  }

  if (intersected) {
    return res;
  }

  res = MAX(res, -p.target);
  for (int32_t i = 0; i < p.collection_len; i++) {
    float dist = -gp_convex_distance(p.polygon, &p.collection[i]);
    res = MAX(res, dist);
  }

  return res;
}