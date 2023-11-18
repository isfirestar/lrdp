#pragma once

struct body
{
    double weight;
    double height;
    double sex;
};

extern unsigned int fetch_training_table(struct body **bodies);
extern struct body *normalize_training_table(struct body *pb, unsigned int num);
extern struct body *normalize_body(struct body *pb);
