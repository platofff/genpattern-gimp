#include <stdlib.h>

#include "basic_geometry.h"

#ifndef _MSC_VER
#define _aligned_free(ptr) free(ptr)
#else
#include <malloc.h>
#endif

void polygon_free(Polygon *p) {
    _aligned_free(p->x_ptr);
    _aligned_free(p->y_ptr);
    free(p);
}