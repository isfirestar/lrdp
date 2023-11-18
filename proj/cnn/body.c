#include "body.h"

#include "math2.h"

#include <stdlib.h>

static struct body g_bodies[] = {
    { 60.3, 1.65, 0.0 },
    { 72.5, 1.82, 1.0 },
    { 68.9, 1.78, 1.0 },
    { 54.4, 1.52, 0.0 },
    { 77.0, 1.80, 1.0 },
    { 70.4, 1.72, 1.0 },
    { 72.8, 1.72, 1.0 },
    { 74.3, 1.71, 1.0 },
    { 74.0, 1.70, 1.0 },
    { 73.2, 1.69, 1.0 },
    { 72.5, 1.68, 1.0 },
    { 71.6, 1.67, 1.0 },
    { 71.0, 1.67, 1.0 },
    { 55.7, 1.60, 0.0 },
    { 56.7, 1.59, 0.0 },
    { 58.0, 1.59, 0.0 },
    { 59.1, 1.58, 0.0 },
    { 60.1, 1.57, 0.0 },
    { 60.8, 1.57, 0.0 },
    { 60.7, 1.57, 0.0 },
    { 69.0, 1.65, 1.0 },
    { 68.1, 1.65, 1.0 },
    { 66.7, 1.64, 1.0 },
    { 65.6, 1.64, 1.0 },
    { 60.3, 1.55, 0.0 },
    { 59.8, 1.54, 0.0 },
    { 58.9, 1.53, 0.0 },
    { 57.9, 1.53, 0.0 },
};
static double aver_weight = 0.0, aver_height = 0.0;

unsigned int fetch_training_table(struct body **bodies)
{
    if (bodies) {
        *bodies = g_bodies;
    }
    return sizeof(g_bodies) / sizeof(g_bodies[0]);
}

struct body *normalize_training_table(struct body *pb, unsigned int num)
{
    double *weights, *heights;
    unsigned int i;

    if (!pb || 0 == num) {
        return NULL;
    }

    weights = (double *)malloc(sizeof(double) * num);
    if (!weights) {
        return NULL;
    }
    heights = (double *)malloc(sizeof(double) * num);
    if (!heights) {
        free(weights);
        return NULL;
    }

    for (i = 0; i < num; ++i) {
        weights[i] = pb[i].weight;
        heights[i] = pb[i].height;
    }

    aver_weight = average_lf(weights, num);
    aver_height = average_lf(heights, num);
    free(weights);
    free(heights);

    for (i = 0; i < num; ++i) {
        pb[i].weight = pb[i].weight - aver_weight;
        pb[i].height = pb[i].height - aver_height;
    }

    return pb;
}

struct body *normalize_body(struct body *pb)
{
    if (!pb) {
        return NULL;
    }

    pb->weight = pb->weight - aver_weight;
    pb->height = pb->height - aver_height;

    return pb;
}
