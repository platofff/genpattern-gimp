#ifndef _GP_BOX_H
#define _GP_BOX_H 1

#include "basic_geometry.h"
#include "misc.h"

#include <stdbool.h>

typedef struct {
  gp_float xmin, ymin, xmax, ymax;
} GPBox;

bool gp_boxes_intersect(GPBox box1, GPBox box2);
gp_float gp_boxes_inner_distance(GPBox box1, GPBox box2);

#endif