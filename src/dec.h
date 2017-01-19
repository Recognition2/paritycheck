//
// Created by gregory on 21-12-16.
//

#ifndef FOURDIMPARITY_DEC_H
#define FOURDIMPARITY_DEC_H

#include "globals.h"
int dec(bool *data, bool *parity);
int correctExtraParity(int totaldiff, bool **cpar, bool **rpar);
int correctOne(bool **m, bool **cpar, bool **rpar);
int correctTwoSeparate(bool **m, bool **cpar, bool **rpar);
void swap (int *a, int *b);

#endif //FOURDIMPARITY_DEC_H
