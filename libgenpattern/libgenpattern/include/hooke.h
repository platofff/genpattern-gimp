#ifndef _GP_HOOKE_H
#define _GP_HOOKE_H 1

#include "basic_geometry.h"
#include "suitability.h"

#define GP_HOOKE_ACCURACY 2.f // TODO
float gp_maximize_suitability(GPPoint b1, float step, float target, GPSParams* sp, GPPoint* res);

#endif