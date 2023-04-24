#include <stdbool.h>

#include "hooke.h"
#include "polygon_translate.h"
#include "misc.h"

static inline float _gp_f(GPSParams* sp, float x, float y) {
  gp_polygon_translate(sp->polygon, sp->ref, x, y);
  float res = gp_suitability(*sp);
  return res;
}

static inline GPVector _gp_first_phase(float b1[2], float *steps, bool* change_steps, GPSParams* sp, float *res) {
  float r1 = _gp_f(sp, b1[0], b1[1]);
  float b2[2] = {b1[0], b1[1]};
  float r2 = r1;
  while (b1[0] == b2[0] && b1[1] == b2[1]) {
    for (int i = 0; i < 2; i++) {
      if (steps[i] < GP_HOOKE_ACCURACY) {
        continue;
      }
      float b[2] = {b2[0], b2[1]};
      b[i] += steps[i];
      float forward = _gp_f(sp, b[0], b[1]);
      b[i] = b2[i] - steps[i];
      float backward = _gp_f(sp, b[0], b[1]);
      if (r1 <= forward && r1 <= backward) {
        if (*change_steps) {
          steps[i] = (int32_t)steps[i] / 2;
        }
      } else if (backward < forward) {
        b2[i] -= steps[i];
        r2 = backward;
      } else {
        b2[i] += steps[i];
        r2 = forward;
      }
    }
    if (!*change_steps && r1 == r2) {
      *change_steps = false;
      *res = r2;
      return (GPVector){b2[0], b2[1]};
    }
    if (steps[0] < GP_HOOKE_ACCURACY && steps[1] < GP_HOOKE_ACCURACY) {
      *change_steps = true;
      *res = r1;
      return (GPVector){b2[0], b2[1]};
    }
  }
  *change_steps = false;
  *res = r2;
  return (GPVector){b2[0], b2[1]};
}

float gp_maximize_suitability(GPPoint b1, float step, float target, GPSParams* sp, GPPoint* res) {
  bool change_steps = false;
  float res2;
  float steps[2] = {step, step};
  while (true) {
    bool stop;
    GPPoint b2 = _gp_first_phase((float[2]){b1.x, b1.y}, steps, &stop, sp, &res2); 
    
    *res = b2;
    _gp_f(sp, b2.x, b2.y);  // TODO
    return res2;

    if (stop || res2 <= target) {
      *res = b2;
      _gp_f(sp, b2.x, b2.y);  // TODO
      return res2;
    }

    while (true) {
      GPVector b3 = {
          .x = b1.x + 2.f * (b2.x - b1.x), .y = b1.y + 2.f * (b2.y - b1.y)
      };
      float res4;
      GPVector b4 = _gp_first_phase((float[2]){b3.x, b3.y}, steps, &change_steps, sp, &res4);
      
      res2 = _gp_f(sp, b2.x, b2.y);
      if (res2 > res4) {
        if (res4 <= target) {
          *res = b4;
          _gp_f(sp, b4.x, b4.y); // TODO
          return res4;
        }
        b1 = b2;
        b2 = b4;
      } else {
        b1 = b2;
        goto next_iter;
      }
    }
  next_iter:;
  }
}