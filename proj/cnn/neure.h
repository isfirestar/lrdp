#pragma once

struct neure_parameter
{
    double w1;
    double w2;
    double w3;
    double w4;
    double w5;
    double w6;
    double b1;
    double b2;
    double b3;
};

extern double sigmino(double x);
extern double sigmino_derivative(double x);

extern double neure_h1(struct neure_parameter *neure, double weight, double height, double *sum);
extern double neure_h2(struct neure_parameter *neure, double weight, double height, double *sum);
extern double neure_o(struct neure_parameter *neure, double weight, double height, double *sum);
extern double neure_feedforward(struct neure_parameter *neure, double weight, double height);

extern struct neure_parameter *neure_allocate();
extern void neure_destroy(struct neure_parameter *neure);

extern void neure_save_parameter(struct neure_parameter *neure, const char *filename);
extern struct neure_parameter *neure_load(const char *filename);
