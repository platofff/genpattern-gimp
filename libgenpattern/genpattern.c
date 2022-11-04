#include "genpattern.h"
#include "test_image1.h"

#include <stdio.h>
#include <sys/time.h>

#ifdef __AVX2__
#include <immintrin.h>
#endif

void bounding_polygon(GEOSGeometry **polygon, ImgAlpha *alpha, int8_t t) {
    t--;
#ifdef __AVX2__
    __m256i cmp_vec = _mm256_set1_epi8(t);
#endif
    size_t buf_size = sizeof(double) * 2 * (alpha->width + alpha->height); // perimeter of the image
    double *x_list = malloc(buf_size),
           *y_list = malloc(buf_size);
    size_t seq_size = 0;

    // a
    int64_t min = 0;
    for (int64_t i = 0; i < alpha->width; i++) {
        int64_t j;
#ifdef __AVX2__
        j = alpha->height - 33;
        for (; j > min + 32; j -= 32) {
            __m256i vec = _mm256_loadu_si256((const __m256i*)&alpha->data[i*alpha->width + j]);
            __m256i result = _mm256_cmpgt_epi8(vec, cmp_vec);
            int32_t cmp = _mm256_movemask_epi8(result);
            if (cmp != 0) {
                j += 31 - __builtin_clz(cmp);
                x_list[seq_size] = i;
                y_list[seq_size++] = j;
                min = j;
                goto continue_a;
            }
        }
        if (j < 0) {
            j = alpha->height - 1;
        }
#else
        j = alpha->height - 1;
#endif
        for (; j > min; j--) {
            if (alpha->data[i*alpha->width + j] > t) {
                x_list[seq_size] = i;
                y_list[seq_size++] = j;
                min = j;
                break;
            }
        }
        continue_a: continue;
    }

    // b
    min = 0;
    for (int64_t j = alpha->height-1; j >= 0; j--) {
        int64_t i = alpha->width - 1;
        for (; i > min; i--) {
            if (alpha->data[i*alpha->width + j] > t) {
                x_list[seq_size] = i;
                y_list[seq_size++] = j;
                min = i;
                break;
            }
        }
    }

    // c
    int64_t max = alpha->height - 1;
    for (int64_t i = alpha->width-1; i >= 0; i--) {
        int64_t j = 0;
#ifdef __AVX2__
        for (; j < max - 32; j += 32) {
            __m256i vec = _mm256_loadu_si256((const __m256i*)&alpha->data[i*alpha->width + j]);
            __m256i result = _mm256_cmpgt_epi8(vec, cmp_vec);
            int32_t cmp = _mm256_movemask_epi8(result);
            if (cmp != 0) {
                j += 33 - __builtin_ctz(cmp);
                x_list[seq_size] = i;
                y_list[seq_size++] = j;
                max = j;
                goto continue_c;
            }
        }
#endif
        for (; j < max; j++) {
            if (alpha->data[i*alpha->width + j] > t) {
                x_list[seq_size] = i;
                y_list[seq_size++] = j;
                max = j;
                break;
            }
        }
        continue_c: continue;
    }

    // d
    max = alpha->width;
    for (int64_t j = 0; j < alpha->height; j++) {
        int64_t i = 0;
        for (; i < max; i++) {
            if (alpha->data[i*alpha->width + j] > t) {
                x_list[seq_size] = i;
                y_list[seq_size++] = j;
                max = i;
                break;
            }
        }
    }

    printf("%zu\n", seq_size);
    GEOSCoordSequence* seq = GEOSCoordSeq_copyFromArrays(x_list, y_list, NULL, NULL, seq_size);
    GEOSGeometry *lring = GEOSGeom_createLinearRing(seq);
    *polygon = GEOSConvexHull(lring);
    GEOSCoordSeq_destroy(seq);
}

int main() {

    struct timeval t0, t1;
    
    GEOSGeometry *polygon;

    gettimeofday(&t0, 0);
    bounding_polygon(&polygon, &test_image, -95);
    gettimeofday(&t1, 0);

    int32_t elapsed = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;
    printf("%d\n", elapsed);

    GEOSGeom_destroy(polygon);
}