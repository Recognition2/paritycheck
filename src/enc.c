//
// Created by gregory on 15-12-16.
//

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "enc.h"

bool *enc (bool *data) {
    // Init
//    const int dsize = MSIZE*MSIZE;                  // Data block size
    const int psize = 6*MSIZE + 2;                  // Parity size
//    const int bsize = dsize + psize;                // Total block size

    // Memory allocation
    bool **matrix = calloc(MSIZE, sizeof(*matrix));    // Data matrix
    for (int i = 0; i < MSIZE; i++)
        matrix[i] = calloc(MSIZE, sizeof(*matrix[i]));    // Sub vectors of data matrix

    // Note: This array is a "jagged array"
    bool **parity = calloc(DIM, sizeof(*parity));      // All parity bits
    for (int i = 0; i < 2; i++) {
        parity[i] = calloc(MSIZE + 1, sizeof(*parity[i]));    // Horz & vert direction
        parity[i + 2] = calloc(MSIZE * 2 - 1, sizeof(*parity[i+2]));    // Diag & anti-diag direction
    }

	// Fill the matrix with data.
	unsigned int howfar = 0;
	for (int i = 0; i < MSIZE; i++)
		for (int j = 0; j < MSIZE; j++)
            matrix[i][j] = data[howfar++];

	// Calculate code horz & vert parity bits
    for (int i = 0; i < MSIZE; i++) {
        for (int j = 0; j < MSIZE; j++) {
            /*
             * For every dimension, in another dimension the sum needs to be calculated.
             * The dimension indicates which rib to use.
             * i indicates how far along that rib we are.
             * j is the dimension that needs to be summed.
             *
             * Furthermore, every parity bit includes exactly 1/MSIZE of all data.
             * All data that has the same coordinate in that dimension.
             */
            parity[0][i] ^= matrix[i][j];      // Horz
            parity[1][i] ^= matrix[j][i];      // Vert

            parity[2][i+j] ^= matrix[i][j];  // Diagonal
            parity[3][MSIZE - 1 - i + j] ^= matrix[i][j];   // Cross diagonal
            // This last one took approx 3 hours to figure out. Really annoying.
            }
    }

    // At this point, the data matrix is no longer needed.
    for (int i = 0; i < MSIZE; i++)
        free(matrix[i]);
    free(matrix);

    // Calculate final parity bits
    for (int i = 0; i < DIM; i++) {
        int max = ((i < 2) ? MSIZE : 2 * MSIZE - 2);
        for (int j = 0; j < max; j++)
            parity[i][max] ^= parity[i][j];
    }

    // Printing stuff because I'm just straight up incompetent, and can't write code that works
//    printf("Printing all parity bits:\n");
//    for (int i = 0; i < DIM; i++) {
//        for (int j = 0; j < ((i < 2) ? MSIZE+1 : 2*MSIZE-1); j++) {
//            printf("%d", parity[i][j]);
//        }
//        printf("\n");
//    }
//    printf("\n");

    // Encode parity data into one long bitstream
    bool *encoded = calloc(psize, sizeof(*encoded));
    howfar = 0;
    for (int i = 0; i < 2; i++)
        for (int j = 0; j <  ((i < 2) ? MSIZE+1 : 2*MSIZE-1); j++)
            encoded[howfar++] = parity[i][j];

    // At this point, the parity bits are no longer needed
    for (int i = 0; i < DIM; i++)
        free(parity[i]);
    free(parity);

    // The encoded data contains ONLY the parity bits.
    return encoded;
}
