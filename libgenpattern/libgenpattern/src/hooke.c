#include <stdbool.h>

#include "hooke.h"
#include "misc.h"
#include "polygon_translate.h"

static inline gp_float _gp_f(GPSParams* sp,
                             GPPolygon* polygons_buffer,
                             GPPolygon** out,
                             int32_t* out_len,
                             gp_float x,
                             gp_float y) {
  gp_polygon_translate(polygons_buffer, sp->ref, x, y);
  return gp_suitability(*sp, polygons_buffer, out, out_len);
}

static inline gp_float _gp_f_cached(GPSParams* sp,
                                    GPSCache* cache,
                                    gp_float x,
                                    gp_float y,
                                    GPPolygon** out,
                                    int32_t* out_len) {
  for (int32_t i = 0; i < GP_HOOKE_CACHE_SIZE; i++) {
    if (i == cache->next && !cache->full) {
      break;
    }
    if (cache->args[i].x == x && cache->args[i].y == y) {
      if (out != NULL) {
        *out = cache->out[i];
        *out_len = cache->out_len[i];
      }
      return cache->results[i];
    }
  }

  gp_float res = _gp_f(sp, &sp->polygons_buffer[5 * cache->next], &cache->out[cache->next],
                       &cache->out_len[cache->next], x, y);
  cache->args[cache->next].x = x;
  cache->args[cache->next].y = y;
  cache->results[cache->next] = res;

  if (out != NULL) {
    *out = cache->out[cache->next];
    *out_len = cache->out_len[cache->next];
  }

  if (cache->next == GP_HOOKE_CACHE_SIZE - 1) {
    cache->next = 0;
    cache->full = true;
  } else {
    cache->next++;
  }

  return res;
}

static inline GPVector _gp_first_phase(gp_float b1[2],
                                       gp_float* steps,
                                       bool* change_steps,
                                       GPSParams* sp,
                                       GPSCache* cache,
                                       gp_float* res) {
  gp_float r1 = _gp_f_cached(sp, cache, b1[0], b1[1], NULL, NULL);
  gp_float b2[2] = {b1[0], b1[1]};
  gp_float r2 = r1;
  while (b1[0] == b2[0] && b1[1] == b2[1]) {
    for (int i = 0; i < 2; i++) {
      if (steps[i] < GP_HOOKE_ACCURACY) {
        continue;
      }
      gp_float b[2] = {b2[0], b2[1]};
      b[i] += steps[i];
      gp_float forward = _gp_f_cached(sp, cache, b[0], b[1], NULL, NULL);
      b[i] = b2[i] - steps[i];
      gp_float backward = _gp_f_cached(sp, cache, b[0], b[1], NULL, NULL);
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

gp_float gp_maximize_suitability(GPPoint b1,
                                 gp_float step,
                                 gp_float target,
                                 GPSParams* sp,
                                 GPPoint* out,
                                 int32_t* out_len,
                                 GPPolygon** out_polygons) {
  GPSCache cache = {0};

  bool change_steps = false;
  gp_float res2;
  gp_float steps[2] = {step, step};
  while (true) {
    bool stop;
    GPPoint b2 = _gp_first_phase((gp_float[2]){b1.x, b1.y}, steps, &stop, sp, &cache, &res2);

    *out = b2;
    return _gp_f_cached(sp, &cache, out->x, out->y, out_polygons, out_len);

    if (stop || res2 <= target) {
      *out = b2;
      return _gp_f_cached(sp, &cache, out->x, out->y, out_polygons, out_len);
    }

    while (true) {
      GPVector b3 = {.x = b1.x + 2 * (b2.x - b1.x), .y = b1.y + 2 * (b2.y - b1.y)};
      gp_float res4;
      GPVector b4 =
          _gp_first_phase((gp_float[2]){b3.x, b3.y}, steps, &change_steps, sp, &cache, &res4);

      res2 = _gp_f_cached(sp, &cache, b2.x, b2.y, NULL, NULL);
      if (res2 > res4) {
        if (res4 <= target) {
          *out = b4;
          return _gp_f_cached(sp, &cache, out->x, out->y, out_polygons, out_len);
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