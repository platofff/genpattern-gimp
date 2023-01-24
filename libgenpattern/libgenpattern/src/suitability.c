#include <math.h>

#include "convex_intersection_area.h"
#include "suitability.h"
#include "convex_distance.h"

float _gp_borders_suitability(GPPolygon* polygon, GPPolygon* canvas) {
  float dist = gp_boxes_inner_distance(canvas->bounds, polygon->bounds);
  if (dist >= 0) {
    return dist;
  }
  bool intersected;
  return gp_convex_intersection_area(canvas, polygon, &intersected) -
         polygon->area;  // TODO: do it without boxes intersection check
}

float gp_suitability(GPPolygon* polygon, GPPolygon* polygons, int32_t polygons_len, GPPolygon* canvas) {
  float res = _gp_borders_suitability(polygon, canvas);
  bool intersected = res <= 0;

  for (int32_t i = 0; i < polygons_len; i++) {
    bool _intersected;
    float intersection = gp_convex_intersection_area(polygon, &polygons[i], &_intersected);
    if (_intersected) {
      intersected = true;
    }
    if (fpclassify(intersection) != FP_ZERO && -intersection < res) {
      res = -intersection;
    }
  }

  if (intersected) {
    return res;
  }

  for (int32_t i = 0; i < polygons_len; i++) {
    float dist = gp_convex_distance(polygon, &polygons[i]);
    if (dist < res) {
      res = dist;
    }
  }

  return res;
}