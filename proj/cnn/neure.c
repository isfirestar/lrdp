#include "neure.h"

#include "math2.h"
#include "ifos.h"

#include <stdlib.h>
#include <stdio.h>

double sigmino(double x)
{
    return 1.0 / (1.0 + exp(-x));
}

double sigmino_derivative(double x)
{
    return sigmino(x) * (1.0 - sigmino(x));
}

double neure_h1(struct neure_parameter *neure, double weight, double height, double *sum)
{
    double s;

    s = 0.0;
    s += neure->w1 * weight;
    s += neure->w2 * height;
    s += neure->b1;
    if (sum) {
        *sum = s;
    }
    return sigmino(s);
}

double neure_h2(struct neure_parameter *neure, double weight, double height, double *sum)
{
    double s;

    s = 0.0;
    s += neure->w3 * weight;
    s += neure->w4 * height;
    s += neure->b2;
    if (sum) {
        *sum = s;
    }
    return sigmino(s);
}

double neure_o(struct neure_parameter *neure, double weight, double height, double *sum)
{
    double s;

    s = 0.0;
    s += neure->w5 * weight;
    s += neure->w6 * height;
    s += neure->b3;
    if (sum) {
        *sum = s;
    }
    return sigmino(s);
}

double neure_feedforward(struct neure_parameter *neure, double weight, double height)
{
    double h1, h2, o;

    h1 = neure_h1(neure, weight, height, NULL);
    h2 = neure_h2(neure, weight, height, NULL);
    o = neure_o(neure, h1, h2, NULL);

    return o;
}

struct neure_parameter *neure_allocate()
{
    struct neure_parameter *neure;
    long seed;

    neure = (struct neure_parameter *)malloc(sizeof(struct neure_parameter));
    if (!neure) {
        return NULL;
    }

    seed = 12357;
    neure->w1 = uniform_random_lf(0.0, 1.0, &seed);
    neure->w2 = uniform_random_lf(0.0, 1.0, &seed);
    neure->w3 = uniform_random_lf(0.0, 1.0, &seed);
    neure->w4 = uniform_random_lf(0.0, 1.0, &seed);
    neure->w5 = uniform_random_lf(0.0, 1.0, &seed);
    neure->w6 = uniform_random_lf(0.0, 1.0, &seed);
    neure->b1 = uniform_random_lf(0.0, 1.0, &seed);
    neure->b2 = uniform_random_lf(0.0, 1.0, &seed);
    neure->b3 = uniform_random_lf(0.0, 1.0, &seed);

    printf("neure: w1=%lf, w2=%lf, w3=%lf, w4=%lf, w5=%lf, w6=%lf, b1=%lf, b2=%lf, b3=%lf\n",
            neure->w1, neure->w2, neure->w3, neure->w4, neure->w5, neure->w6, neure->b1, neure->b2, neure->b3);

    return neure;
}

void neure_destroy(struct neure_parameter *neure)
{
    if (neure) {
        free(neure);
    }
}

struct neure_parameter *neure_load(const char *filename)
{
    struct neure_parameter *neure;
    file_descriptor_t fd;
    nsp_status_t status;

    neure = (struct neure_parameter *)malloc(sizeof(struct neure_parameter));
    if (!neure) {
        return NULL;
    }

    status = ifos_file_open(filename, FF_OPEN_EXISTING | FF_RDACCESS, 0644, &fd);
    if (!NSP_SUCCESS(status)) {
        free(neure);
        return NULL;
    }

    ifos_file_read(fd, neure, sizeof(struct neure_parameter));
    ifos_file_close(fd);

    return neure;
}

void neure_save_parameter(struct neure_parameter *neure, const char *filename)
{
    file_descriptor_t fd;
    nsp_status_t status;

    status = ifos_file_open(filename, FF_CREATE_ALWAYS | FF_WRACCESS, 0644, &fd);
    if (!NSP_SUCCESS(status)) {
        return;
    }

    ifos_file_write(fd, neure, sizeof(struct neure_parameter));
    ifos_file_close(fd);
}
