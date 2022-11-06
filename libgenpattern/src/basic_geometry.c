#include <stdlib.h>

#include "basic_geometry.h"

void polygon_free(Polygon *p) {
    free(p->x_ptr);
    free(p->y_ptr);
    free(p);
}