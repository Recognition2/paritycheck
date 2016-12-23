//
// Created by gregory on 21-12-16.
//

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#include "dec.h"
#include "enc.h"


bool *dec (bool *data, bool *parity) {
    /*
     * Received data consists of both parity bits and data.
     * As such, these need to be seperated first.
     * Then, the data needs to be encoded, and compared to the result, which is the difficult part.
     * Viterbi decoding will be used for determining what to do, and an error message will be returned if things go
     * so wrong that everything goes to fucky
     */

    if (powl((long double)MSIZE, (long double) DIM) > 4294967294) {
        fprintf(stderr, "Maximum block size has been overridden!\n");
        exit(1);
    }

    // First, declarations
//    const int dsize = (int) pow(MSIZE,DIM);                       // in bits, size the data block
//    const int bsize = dsize + (MSIZE * DIM) + 1;				    // in bits, amount of data in 1 block
//    int psize = bsize - dsize;                              // Amount of parity bits per block


    bool *enc_computed;             // The parity code that is computed, dwz should be.
    enc_computed = enc(data);
    bool cparity[DIM][MSIZE];       // Computed parity bits
    bool rparity[DIM][MSIZE];       // Read parity bits
    unsigned int howfar = 0;             //TODO: If code block is more than 4 GB long, then all will fail

    // Fill in both parity blocks, to be compared later on.
    for (int i = 0; i < DIM; i++) {
        for (int j = 0; j < MSIZE; j++) {
            cparity[i][j] = enc_computed[howfar];
            rparity[i][j] = parity[howfar];
            howfar++;
        }
    }

    // Walk through both matrices, check the amount of differences.
    // If zero, no error occurred (or SO MANY errors occurred, that's not worth mentioning)
    // If exactly 1, then 1 parity bit flipped. Everything is fine.
    // If exactly DIM, then 1 data bit flipped. Correct this, and everything is fine.

    // nonMatchPerDim stores the amount of differences in every dimension. Totaldiff is the sum of all parity bits.
    int *nonMatchPerDim;
    nonMatchPerDim = calloc(DIM, sizeof(int));
    int totaldiff = 0;
    for (int i = 0; i < DIM; i++) {
        for (int j = 0; j < MSIZE; j++) {
            bool isDiff = cparity[i][j] != rparity[i][j];      // Is there a difference in the parity bits?
            nonMatchPerDim[i] += isDiff;
            totaldiff += isDiff;
        }
    }

    if (totaldiff == 0 || totaldiff == 1) {
        // Zero errors occurred, everything is splendid
        // OR: One error occurred, the parity bit itself is in error and correction is not necessary.
        // Return the whole block. This includes the parity bits, but the caller should expect (and handle) only the data anyway.
        return data;
    } else if (totaldiff == 4) {
        // Multiple options possible. The easiest is that exactly 1 data bit flipped, thus flipping four parity bits (one in every dimension).
        // First figure out which data bit is in error, correct it, and then return the block.
        bool onlyOneError = true;    // Has only one error has occurred in only dimension? Let's assume so.
        for (int i = 0; i < DIM; i++) {
            if (nonMatchPerDim[i] != 1) {
                onlyOneError = false;
                break;
            }
        }
        if (onlyOneError) {
            // We are in luck, only one error in every dimension.
            // Correction fairly simple: flip the one data bit that is incorrect.
            int wrong[DIM]; // Coordinates that are wrong
            for (int i = 0; i < DIM; i++) {
                for (int j = 0; j < MSIZE; j++) {
                    if (cparity[i][j] != rparity[i][j]) {      // Is there a difference in the parity bits?
                        wrong[i] = j;
                    }
                }
            }
            // Which data bit does this correspond to in the original array of booleans?
            howfar = 0;
            for (int i = 0; i < DIM; i++) {
                howfar += wrong[0] * pow(DIM,i);
            }

            // Flip this bit
            data[howfar] = !data[howfar];
        } else {
            // We are not in luck. While a total amount of 4 errors has occurred, they are not spread nicely.
            // What the hell has happened?
            // TODO: What happened
        }

    } else { // totaldiff is not 0,1,4
        // Decoding could not be done successfully, thus failure is the only option
        return NULL;
    }

    free(enc_computed);
    free(nonMatchPerDim);
    return NULL;
}
