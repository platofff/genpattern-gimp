#pragma once
#include <stdint.h>

typedef struct {
    float x;
    float y;
} Point;

typedef struct {
    float *x_ptr;
    float *y_ptr;
    int32_t size;
} Polygon;

void polygon_free(Polygon *p);
