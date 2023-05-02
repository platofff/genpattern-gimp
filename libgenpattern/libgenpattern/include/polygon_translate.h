#ifndef _GP_POLYGON_TRANSLATE
#define _GP_POLYGON_TRANSLATE 1

#include "polygon.h"

void gp_polygon_translate(GPPolygon* dst_polygon, GPPolygon* src_polygon, gp_float x_off_val, gp_float y_off_val);
void gp_wrap_polygon_part(GPPolygon* polygon, int area_type, gp_float xres, gp_float yres);

#endif