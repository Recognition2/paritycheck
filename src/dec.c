//
// Created by gregory on 21-12-16.
//

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#include "dec.h"
#include "enc.h"

int dec (bool *data, bool *parity) {
    /*
     * Received data consists of both parity bits and data.
     * As such, these need to be seperated first.
     * Then, the data needs to be encoded, and compared to the result, which is the difficult part.
     * First, we need to establish what type of error has occurred.
     * Then, correction needs to take place.
     */
    const int TOTAL_ERROR = 10000;
//    const int dsize = MSIZE*MSIZE;              // Data block size
    const int psize = 6*MSIZE + 3;              // Parity size
//    const int bsize = dsize + psize;            // Total block size

    bool *enc_computed = enc(data);             // The parity code that is computed, dwz should be.
    int nonConformDim;  // Amount of non-conforming dimension

    // Memory allocation
    bool **matrix = calloc(MSIZE, sizeof(*matrix));    // Data matrix
    for (int i = 0; i < MSIZE; i++)
        matrix[i] = calloc(MSIZE, sizeof(*matrix[i]));    // Sub vectors of data matrix


    // Note: These arrays are "jagged arrays"
    bool **cpar = calloc(DIM, sizeof(*cpar));      // All parity bits
    bool **rpar = calloc(DIM, sizeof(*rpar));      // All parity bits
    for (int i = 0; i < 2; i++) {
        cpar[i] = calloc(MSIZE + 1, sizeof(cpar[i]));    // Horz & vert direction
        cpar[i + 2] = calloc(MSIZE * 2 - 1, sizeof(cpar[i]));    // Diag & anti-diag direction
        rpar[i] = calloc(MSIZE + 1, sizeof(rpar[i]));    // Horz & vert direction
        rpar[i + 2] = calloc(MSIZE * 2 - 1, sizeof(rpar[i]));    // Diag & anti-diag direction
    }

    int howfar = 0;

    // Fill in both parity blocks, to be compared later on.
    for (int i = 0; i < DIM; i++) {
        for (int j = 0; j < ((i < 2) ? MSIZE : 2 * MSIZE - 2); j++) {
            cpar[i][j] = enc_computed[howfar];
            rpar[i][j] = parity[howfar];
            howfar++;
            if (howfar > psize)
                fprintf(stderr,"NO NO NO NO NO");

        }
    }

    // Fill the matrix with data.
    howfar = 0;
    for (int i = 0; i < MSIZE; i++)
        for (int j = 0; j < MSIZE; j++)
            matrix[i][j] = data[howfar++];

    // Walk through both matrices, check the amount of differences.
    // If zero, no error occurred (or SO MANY errors occurred, that's not worth mentioning)
    // If exactly 1, then 1 parity bit flipped. Everything is fine.
    // If exactly DIM (+- 1), then 1 data bit flipped. Correct this.

    // nonMatchPerDim stores the amount of differences in every dimension. Totaldiff is the sum of all parity bits.
    int *nonMatchPerDim = calloc(DIM, sizeof(*nonMatchPerDim));
    int totaldiff = 0;      // Total difference in all dimensions
    int wrongdim = 0;       // Which dimension is the off-by-one one?
    int whichEl[4];    // In all dimensions, the how-many'th of the parity bits is flipped?
    int dims[] = {0,0};     // Two "good" dimensions.
    int w, x, y, z;    // x,y position of the wrong bits

    // Determine which one
    for (int i = 0; i < DIM; i++) {
        for (int j = 0; j < ((i < 2) ? MSIZE : 2 * MSIZE - 2); j++) {
            bool isDiff = (cpar[i][j] != rpar[i][j]);      // Is there a difference in the parity bits?
            nonMatchPerDim[i] += isDiff;                    // The parity of parities is excluded
            totaldiff += isDiff;
            if (isDiff) {
                whichEl[i] = j;
            }
        }
    }
    switch (totaldiff) {
        case 0:
        case 1:
        case 2:
                /*
                 * Zero errors occurred, everything is splendid
                 * OR: One error occurred, the parity bit itself is in error and correction is not necessary.
                 * OR: Two parity bits flipped. Still no cause for concern.
                 * This really is the best case scenario. It's easy and simple and great
                 */
            break;

        case 3:
        case 5:
            /*
             * One data error has occurred. One parity bit flipped back, or an extra one flipped.
             *
             * First, make sure that in all dimensions (except 1) exactly 1 error has occurred.
             */
            nonConformDim = 0;    // Has only one error has occurred in all dimension? Let's check
            for (int i = 0; i < DIM; i++)
                if (nonMatchPerDim[i] != 1)
                    nonConformDim++;

            if (nonConformDim > 1) {
                totaldiff = TOTAL_ERROR;
                goto FREE;
            }
            for (int i = 0; i < DIM; i++)
                if (nonMatchPerDim[i] != 1)
                    wrongdim = i;
            dims[0] = (wrongdim + 1) % 4;
            dims[1] = (wrongdim + 2) % 4;
            // NO BREAK, use "case fallthrough" since both cases are roughly equal, the 3/5 one just needs more love
        case 4:
            /*
             * Now, we can assume it's a 'nice' error that we're dealing with. Correction is straightforward:
             * Exclude the "wrong" dimension, if there is one, and use two others for positioning.
             * Check with the third available dimension. If there is no wrong dimension, also check the fourth dim.
             */
            for (int i = 0; i < DIM; i++) {
                int tmp = ((i < 2) ? MSIZE : 2 * MSIZE - 2);
                if (cpar[i][tmp] != rpar[i][tmp] || nonMatchPerDim[i] != 1) { // Check  final parity bits
                    totaldiff = TOTAL_ERROR;                             // & amount of errors in normal parity bits
                    goto FREE;
                }
            }
            // Start with dimensions 0 and 1, horz & vert. Because it's easy.
            if (dims[0] == dims[1]) { // Switch case is 4.
                dims[0] = 0;
                dims[1] = 1;
            } else {
                // "Good" dimensions are not to be chosen freely. Unfortunately.
                switch (wrongdim) {
                    case 0: // Fuck! i & j need to be reverse engineered.
                        x = whichEl[2] - whichEl[1];
                        if (x == whichEl[0] || x == -1) // Something strange is going on, impossible
                            fprintf(stderr,"This should never happen, x = %d", x);
                        break;
                    case 1:
                        y = whichEl[2] - whichEl[0];

                    default: break;
                }
            }
            // In these dimensions, i & j are given directly, and don't need to be converted to other dimensions.
            // If all is well, whichEl contains the values in all dimensions.
            // First check if all things correspond to the same bit
            // Diagonal:
            bool isdiagCorrect = whichEl[0] + whichEl[1] == whichEl[2];
            bool iscrossCorrect = (MSIZE - 1 - whichEl[0] + whichEl[1]) == whichEl[3];
            if (isdiagCorrect || iscrossCorrect) {
                totaldiff = TOTAL_ERROR;
                goto FREE;
            }
            matrix[whichEl[0]][whichEl[1]] = !matrix[whichEl[0]][whichEl[1]]; // Actual flipping

            break;

        default:
            totaldiff = TOTAL_ERROR;
            break;
    }


    // Free everything, we're not in the Roman empire anymore
    FREE:
    for (int i = 0; i < DIM; i++) {
        free(cpar[i]);
        free(rpar[i]);
    }
    free(cpar);
    free(rpar);
    free(enc_computed);
    free(nonMatchPerDim);
    return totaldiff;
}