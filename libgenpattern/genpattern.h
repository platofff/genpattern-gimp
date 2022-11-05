#include <stdlib.h>
#include <stdint.h>
#include <stdalign.h>
#include <threads.h>

#include <geos_c.h>

typedef struct {
    size_t width;
    size_t height;
    alignas(32) int8_t *data;
} ImgAlpha;

typedef struct {
    GEOSGeometry *polygon;
    ImgAlpha **alpha_ptrs;
    int8_t t;
    uint64_t *next_work;
    mtx_t *next_work_mtx;
    size_t thread_id;
} ConvexParams;
