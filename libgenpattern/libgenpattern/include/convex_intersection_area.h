#ifndef _GP_POLYGON_INTERSECTION_AREA_H
#define _GP_POLYGON_INTERSECTION_AREA_H 1

#include "polygon.h"

void gp_convex_intersection_area(const GPPolygon* polygon1,
                                 const GPPolygon* polygon2,
                                 bool* intersected,
                                 float* res_area,
                                 GPPolygon* res_polygon);

#endif