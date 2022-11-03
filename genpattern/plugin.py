import random
import copy
from itertools import product

import numpy as np
from shapely.geometry import Polygon, MultiPolygon
from shapely.ops import unary_union
from shapely.affinity import translate

from gimpfu import *

from hooke import hooke

import os
if os.name == 'nt':
    from gettext import gettext as lg
else:
    from locale import gettext as lg

class Filled:
    same = []

    def append(self, polygon):
        self.mp = unary_union([self.mp, polygon])
        self.same.append(polygon)

    def __init__(self, w, h):
        w4 = w * 4
        h4 = h * 4
        self.mp = MultiPolygon([Polygon([(-w4, -h4), (w4, -h4), (w4, h4), (-w4, h4)],
                                        holes=[[(0, 0), (0, h), (w, h), (w, 0)]])])

def get_random_positions(iw, ih, grid_resolution):
    res = list(product(range(0, iw, grid_resolution), range(0, ih, grid_resolution)))
    random.shuffle(res)
    return res

def bounding_polygon(alpha, threshold, tolerance, buf_radius):
    # in some reason alpha is already transposed
    res_b = []
    _min = 0
    for j in range(len(alpha) - 1, -1, -1):
        r = np.argmax(alpha[j][_min:][::-1] >= threshold)
        i = len(alpha[j]) - 1 - r
        if r != 0 or alpha[j][i] >= threshold:
            res_b.append((i, j))
            _min = i

    res_d = []
    _max = len(alpha)
    for j in range(len(alpha)):
        i = np.argmax(alpha[j][:_max+1] >= threshold)
        if i != 0 or alpha[j][i] >= threshold:
            res_d.append((i, j))
            _max = i
    
    alpha = alpha.transpose()

    res_a = []
    _min = 0
    for i in range(len(alpha)):
        r = np.argmax(alpha[i][_min:][::-1] >= threshold)
        j = len(alpha[i]) - 1 - r
        if r != 0 or alpha[i][j] >= threshold:
            res_a.append((i, j))
            _min = j

    res_c = []
    _max = len(alpha[0])
    for i in range(len(alpha) - 1, -1, -1):
        j = np.argmax(alpha[i][:_max+1] >= threshold)
        if j != 0 or alpha[i][j] >= threshold:
            res_c.append((i, j))
            _max = j

    res = res_a + res_b + res_c + res_d
    if len(res) < 3:
        return None
    
    res.append(copy.copy(res[0]))
    j = 0
    
    while True:
        p = (res[j+1][1] - res[j][1]) * (res[j+2][0] - res[j+1][0]) - (res[j+1][0] - res[j][0]) * (res[j+2][1] - res[j+1][1])
        if p > 0:
            if j + 3 == len(res):
                break
            j += 1
        else:
            res.pop(j + 1)
            if j > 0:
                j -= 1
            else:
                j += 1
    
    return Polygon(res).buffer(buf_radius).simplify(tolerance)


def polygon_intersection(filled, c_polygon):
    return filled.mp.intersection(c_polygon).area


def min_distance_from_same(filled, p):
    return min(polygon.distance(p) for polygon in filled.same)

def place_polygon(p, iw, ih, grid_resolution, initial_step, target, force_closer, accuracy, pcb, filled):
    def check_pos(x, y):
        polygon = translate(p, x, y)
        res = polygon_intersection(filled, polygon)
        if res == 0:
            if len(filled.same) == 0:
                res = -target
            else:
                res = -min_distance_from_same(filled, polygon)
        return res

    best = (0, None)
        
    positions = get_random_positions(iw, ih, grid_resolution)
    s = len(positions)

    while len(positions) != 0:
        pos, res = hooke(check_pos, list(positions.pop(0)), initial_step, accuracy, -target)
        progress = s - len(positions)
        if progress % 10 == 0:
            pcb(progress, s)

        if res <= -target:
            polygon = translate(p, *pos)
            filled.append(polygon)
            return pos
        if best[0] > res:
            best = (res, pos)

    if force_closer and best[1] is not None:
        polygon = translate(p, *best[1])
        filled.append(polygon)
        return best[1]

def randomize_variants(hr):
    return [0, 1] if hr else [0]


def randomize_layer(layer, variants, la, ha):
    pdb.gimp_drawable_transform_rotate_default(layer, np.deg2rad(random.randint(la, ha)),
                                               True, 0, 0, True, TRANSFORM_RESIZE_ADJUST)
    variant = random.choice(variants)

    if variant == 1:
        pdb.gimp_drawable_transform_matrix(layer,
                                           -1, 0, 0,
                                           0, 1, 0,
                                           0, 0, 1,
                                           TRANSFORM_FORWARD, INTERPOLATION_NONE,
                                           False, 1, TRANSFORM_RESIZE_ADJUST)


def layer_data(layer):
    region = layer.get_pixel_rgn(0, 0, layer.width, layer.height)
    return np.frombuffer(region[:, :], dtype=np.uint8).reshape(layer.height, layer.width, 4)


def gen_pattern(img, threshold, copies, allow_reflection, buf_diameter, grid_resolution, max_angle,
                          simp_level, initial_step, min_distance, force_closer, accuracy, progress_callback):
    filled = Filled(img.width, img.height)

    pdb.gimp_image_undo_group_start(img)
    lt = {}
    rv = randomize_variants(allow_reflection)
    buf_radius = buf_diameter // 2

    def get_polygon(layer):
        randomize_layer(layer, rv, -max_angle, max_angle)
        alpha = layer_data(layer)[:, :, 3]
        return bounding_polygon(alpha, threshold, simp_level, buf_radius)

    count = 0
    total = sum(copies)
    for i, layer in enumerate(img.layers):
        if copies[i] == 0:
            img.remove_layer(layer)
            continue
        p = get_polygon(layer)
        if p is None:
            continue
        lt.update({i: [(layer, p)]})
        count += 1
        progress_callback('%s %d' % (lg('Computed bounding polygon for layer'), count), float(count) / total)
        for _ in range(copies[i] - 1):
            new_layer = pdb.gimp_layer_copy(layer, 0)
            new_layer.set_offsets(img.width - new_layer.width, img.height - new_layer.height)
            pdb.gimp_image_insert_layer(img, new_layer, None, 0)
            lt[i].append((new_layer, get_polygon(new_layer)))
            count += 1
            progress_callback('%s %d' % (lg('Computed bounding polygon for layer'), count), float(count) / total)

    count = 0
    for_removing = []

    for layers in sorted(lt.keys(), key=lambda x: lt[x][0][1].area, reverse=True):
        filled.same = []
        for layer, polygon in lt[layers]:
            count += 1
            progress = float(count) / total
            pos = place_polygon(polygon, img.width, img.height, grid_resolution, initial_step, min_distance, force_closer,
                                accuracy, lambda s, o: progress_callback('%s %d %s %d' % (lg('Trying in grid node'), s, lg('of'), o), progress), filled)
            if pos == None:
                progress_callback('%s %d %s %d' % (lg('Failed to place'), count, lg('of'), total), progress)
                for_removing.append(layer)
                continue
            layer.set_offsets(pos[0], pos[1])
            progress_callback('%s %d %s %d' % (lg('Placed'), count, lg('of'), total), float(count) / total)

    for layer in for_removing:
        img.remove_layer(layer)
    pdb.gimp_image_undo_group_end(img)
