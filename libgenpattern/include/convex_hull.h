#ifndef CONVEX_HULL_H
#define CONVEX_HULL_H 1

#include <stdlib.h>
#include <stdint.h>
#include <stdalign.h>

#include "basic_geometry.h"

typedef struct {
    size_t width;
    size_t height;
    alignas(32) uint8_t *data;
} ImgAlpha;

void image_convex_hull(Polygon **polygon, ImgAlpha *alpha, uint8_t _t);
#endif
