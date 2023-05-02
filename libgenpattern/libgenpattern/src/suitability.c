#include <assert.h>
#include <math.h>

#include "convex_distance.h"
#include "convex_intersection.h"
#include "misc.h"
#include "polygon_translate.h"
#include "suitability.h"


gp_float gp_suitability(GPSParams p, GPPolygon* polygons_buffer, GPPolygon** out, int32_t* out_len) {
  *out_len = 1;

  GPPolygon* pb = polygons_buffer;
  *out = pb;

  for (int i = 0; i < 8; i++) {
    bool intersected;
    pb[*out_len].size = 0;
    pb[*out_len].bounds.xmin = INFINITY;
    pb[*out_len].bounds.ymin = INFINITY;
    pb[*out_len].bounds.xmax = -INFINITY;
    pb[*out_len].bounds.ymax = -INFINITY;
    gp_convex_intersection(&p.canvas_outside_areas[i], &pb[0], &intersected, NULL, &pb[*out_len]);
    if (pb[*out_len].size != 0) {
      pb[*out_len].base_offset.x = pb[0].base_offset.x;
      pb[*out_len].base_offset.y = pb[0].base_offset.y;
      gp_wrap_polygon_part(&pb[*out_len], i, p.canvas->bounds.xmax, p.canvas->bounds.ymax);
      *out_len = *out_len + 1;
      assert(*out_len <= 4);
    }
  }
  if (*out_len > 1) {
    gp_convex_intersection(p.canvas, &pb[0], NULL, NULL, &pb[*out_len]);
    pb++;
    *out = pb;
    *out_len = *out_len - 1;
  }


  bool intersected = false;
  gp_float res = 0;

  for (int32_t j = 0; j < *out_len; j++) {
    for (int32_t i = 0; i < p.polygons_len; i++) {
      bool _intersected;
      gp_float intersection_area;
      gp_convex_intersection(&pb[j], &p.polygons[i], &_intersected, &intersection_area, NULL);
      if (_intersected) {
        if (!intersected) {
          intersected = true;
          res = 0;
        }
        res += intersection_area;
      }
    }
  }

  if (intersected) {
    return res;
  }

  res = -p.target;

  for (int32_t j = 0; j < *out_len; j++) {
    for (int32_t i = 0; i < p.collection_len; i++) {
      gp_float dist = -gp_convex_distance(&pb[j], &p.collection[i]);
      res = GP_MAX(res, dist);
    }
  }

  return res;
}