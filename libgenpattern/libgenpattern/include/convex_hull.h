#ifndef _GP_CONVEX_HULL_H
#define _GP_CONVEX_HULL_H 1

#include <stdint.h>
#include <stdlib.h>

#include "polygon.h"

typedef struct {
  size_t width;
  size_t height;
  uint8_t* data;
} GPImgAlpha;

void gp_image_convex_hull(GPPolygon* polygon, GPImgAlpha* alpha, uint8_t _t, int32_t collection_id);

#endif