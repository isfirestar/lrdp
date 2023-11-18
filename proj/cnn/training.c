#include "neure.h"
#include "body.h"

#include "math2.h"
#include "lobj.h"

#include <stdio.h>
#include <stdlib.h>

static double MES_derivative(double y_predict, double y_actual)
{
    return -2 * (y_predict - y_actual);
}

void training_oneshot(const struct body *bp, struct neure_parameter *neure, double learn_rate)
{
    double sum_h1, h1, sum_h2, h2, sum_o, o;
    double y_predict;
    double d_L_d_ypred, d_ypred_d_w5, d_ypred_d_w6, d_ypred_d_b3;
    double d_ypred_d_h1, d_ypred_d_h2;
    double d_h1_d_w1, d_h1_d_w2, d_h1_d_b1;
    double d_h2_d_w3, d_h2_d_w4, d_h2_d_b2;

    h1 = neure_h1(neure, bp->weight, bp->height, &sum_h1);
    h2 = neure_h2(neure, bp->weight, bp->height, &sum_h2);
    o = neure_o(neure, h1, h2, &sum_o);
    y_predict = o;

    // 以下开始计算偏导数, 以MES算法为损失函数
    d_L_d_ypred = MES_derivative(bp->sex, y_predict);

    // 神经元 o
    d_ypred_d_w5 = h1 * sigmino_derivative(sum_o);
    d_ypred_d_w6 = h2 * sigmino_derivative(sum_o);
    d_ypred_d_b3 = sigmino_derivative(sum_o);

    d_ypred_d_h1 = neure->w5 * sigmino_derivative(sum_o);
    d_ypred_d_h2 = neure->w6 * sigmino_derivative(sum_o);

    // 神经元 h1
    d_h1_d_w1 = bp->weight * sigmino_derivative(sum_h1);
    d_h1_d_w2 = bp->height * sigmino_derivative(sum_h1);
    d_h1_d_b1 = sigmino_derivative(sum_h1);

    // 神经元 h2
    d_h2_d_w3 = bp->weight * sigmino_derivative(sum_h2);
    d_h2_d_w4 = bp->height * sigmino_derivative(sum_h2);
    d_h2_d_b2 = sigmino_derivative(sum_h2);

    // 更新权重w和偏移量b
    // 神经元 h1
    neure->w1 -= learn_rate * d_L_d_ypred * d_ypred_d_h1 * d_h1_d_w1;
    neure->w2 -= learn_rate * d_L_d_ypred * d_ypred_d_h1 * d_h1_d_w2;
    neure->b1 -= learn_rate * d_L_d_ypred * d_ypred_d_h1 * d_h1_d_b1;

    // 神经元 h2
    neure->w3 -= learn_rate * d_L_d_ypred * d_ypred_d_h2 * d_h2_d_w3;
    neure->w4 -= learn_rate * d_L_d_ypred * d_ypred_d_h2 * d_h2_d_w4;
    neure->b2 -= learn_rate * d_L_d_ypred * d_ypred_d_h2 * d_h2_d_b2;

    // 神经元 o
    neure->w5 -= learn_rate * d_L_d_ypred * d_ypred_d_w5;
    neure->w6 -= learn_rate * d_L_d_ypred * d_ypred_d_w6;
    neure->b3 -= learn_rate * d_L_d_ypred * d_ypred_d_b3;
}

double loss(struct neure_parameter *neure, const struct body *bodies, unsigned int num)
{
    unsigned int i;
    double *y_predict, *y_actual, lossed;

    y_predict = (double *)malloc(sizeof(double) * num);
    if (!y_predict) {
        return 0.0;
    }
    y_actual = (double *)malloc(sizeof(double) * num);
    if (!y_actual) {
        free(y_predict);
        return 0.0;
    }

    for (i = 0; i < num; i++) {
        y_actual[i] = bodies[i].sex;
        y_predict[i] = neure_feedforward(neure, bodies[i].weight, bodies[i].height);
    }

    lossed = mse_lf(y_predict, y_actual, num);
    printf("MES loss: %lf\n", lossed);

    free(y_predict);
    free(y_actual);
    return lossed;
}

void training()
{
    struct body *bodies, *pb;
    unsigned int num, i, j;
    struct neure_parameter *neure;

    num = fetch_training_table(&bodies);
    pb = normalize_training_table(bodies, num);

    neure = neure_allocate();

    for (i = 0; i < 10; i++) {
        for (j = 0; j < num; j++) {
            training_oneshot(&pb[j], neure, 0.1);
        }
        loss(neure, pb, num);
    }

    neure_save_parameter(neure, "neure.dat");
}

void predict(double weight, double height)
{
    struct body b, *pb;
    double o;
    struct neure_parameter *neure;

    neure = neure_load("neure.dat");
    if (!neure) {
        printf("neure load failed, try to training it at first\n");
        return;
    }

    b.weight = weight;
    b.height = height;

    pb = normalize_body(&b);

    o = neure_feedforward(neure, pb->weight, pb->height);
    printf("predict sex: %lf\n", o);
}

int cnn_post_init(lobj_pt lop, int argc, char **argv)
{
    if (0 == argc) {
        return 1;
    }

    if (0 == strcasecmp(argv[0], "training")) {
        training();
    }

    if (0 == strcasecmp(argv[0], "predict")) {
        if (argc < 3) {
            printf("usage: %s predict weight height\n", argv[0]);
            return 1;
        }
        predict(atof(argv[1]), atof(argv[2]));
    }

    return 1;
}
