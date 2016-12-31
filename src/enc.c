//
// Created by gregory on 15-12-16.
//

#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#include "enc.h"

bool* enc (bool* data) {
	// First, check whether the block size is larger than would fit into 4 bytes
	if (powl((long double)MSIZE, (long double) DIM) > 4294967294) {
		fprintf(stderr, "Maximum block size has been overridden!\n");
		exit(EXIT_FAILURE);
	}

	int dsize = (int) pow(MSIZE,DIM);						// in bits, size the data block
	int bsize = dsize + 2*(MSIZE * DIM) + 1;				// in bits, amount of data in 1 block
//    int psize = bsize - dsize;                                  // Amount of parity bits per block

	bool m[MSIZE][MSIZE][MSIZE][MSIZE];
	int parity[DIM][MSIZE] = {0};									// All DEC parity bits
	int sum = 0;

	// Fill the matrix with data.
	unsigned int howfar = 0;			//TODO: If code is more than 4 GB long, this will fail.
	for (int i = 0; i < MSIZE; i++) {
		for (int j = 0; j < MSIZE; j++) {
			for (int k = 0; k < MSIZE; k++) {
				for (int l = 0; l < MSIZE; l++) {
					m[i][j][k][l] = data[howfar]; // Assign data to matrix

					/*
					 * The last parity bit is calculated by using ALL data bits, and all parity bits.
					 * The first part is done here, because I've got this four-deep nested loop going already.
					 */
					sum += data[howfar] % 4;
                    howfar++;
				}
			}
		}
	}

	// Calculate code parity bits, every dimension separately
	for (int i = 0; i < MSIZE; i++) {
		// in 1 dimension
		// Every parity bit includes exactly 1/MSIZE of all data: all data that has the same coordinate in that dimension.
		// As such, the data must be walked through in every case
		for (int j = 0; j < MSIZE; j++) {
			for (int k = 0; k < MSIZE; k++) {
				for (int l = 0; l < MSIZE; l++) {
				/*
				 * For every dimension, in another dimension the sum needs to be calculated.
				 * The dimension indicates which rib to use.
				 * i indicates how far along that rib we are.
				 * j,k,l are the dimensions which need to be summed. For every dimension, this calculation
				 * is performed separately, because automating this is not trivial, and this does not exceed boundaries.
				 */
					parity[0][i] += m[i][j][k][l];
					parity[1][i] += m[l][i][j][k];
					parity[2][i] += m[k][l][i][j];
					parity[3][i] += m[j][k][l][i];
				}
			}
		}
	}

	/*
	 * Reassign the parity matrix to a simple binary vector that can be returned
	 * While we're doing this, sum the parity bits, because the loop is there anyway.
	 */
	howfar = 0;
    bool *encoded = calloc((size_t) bsize - dsize, sizeof(bool));		// Encoded data, "result" of function
    for (int i = 0; i < DIM; i++) {
		for (int j = 0; j < MSIZE; j++) {
            bool *tmp = encodeGray(parity[i][j]);
            encoded[2*howfar] = tmp[0];
            encoded[2*howfar + 1] = tmp[1];
			sum += parity[i][j];
            howfar++;
            free(tmp);
		}
	}

	// The last parity bit, calculation of which has just been completed, now needs to be added to array that will be returned.
	encoded[howfar] = (sum % 2) != 0;

    // Printing stuff because I'm just straight up incompetent, and can't write code that works
//    for (int i = 0; i < psize; i++) {
//        printf("%d", encoded[i]);
//        if ((i % 4) == 3) {
//            printf("\n");
//        }
//    }
//    printf("\n");

    // The encoded data contains ONLY the parity bits.
    return encoded;
}

bool *encodeGray (int number) {
    bool *gray;
    gray = calloc(2, sizeof(bool));
    u_int8_t mask = 0x1;
    int num = number%4;
    gray[0] = (bool) (num & (mask<<1));     // Shifted number
    gray[1] = (bool) ((num & (mask<<1)) ^ (num & mask));    // XOR of both bits

    return gray;
}