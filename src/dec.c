//
// Created by gregory on 21-12-16.
//

#include <stdio.h>
#include <stdlib.h>
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
//    const int dsize = MSIZE*MSIZE;              // Data block size
    const int psize = 6*MSIZE + 2;              // Parity size
//    const int bsize = dsize + psize;            // Total block size

    bool *enc_computed = enc(data);             // The parity code that is computed, dwz should be.

    // Memory allocation
    bool **matrix = calloc(MSIZE, sizeof(*matrix));    // Data matrix
    for (int i = 0; i < MSIZE; i++)
        matrix[i] = calloc(MSIZE, sizeof(*matrix[i]));    // Sub vectors of data matrix


    // Note: These arrays are "jagged arrays"
    bool **cpar = calloc(DIM, sizeof(*cpar));      // All parity bits
    bool **rpar = calloc(DIM, sizeof(*rpar));      // All parity bits
    for (int i = 0; i < 4; i++) {
        size_t size = (i < 2) ? MSIZE+1 : 2*MSIZE;
        cpar[i] = calloc(size, sizeof(bool));
        rpar[i] = calloc(size, sizeof(bool));
    }

    int howfar = 0;

    // Fill in both parity blocks, to be compared later on.
    for (int i = 0; i < DIM; i++) {
        for (int j = 0; j < ((i < 2) ? MSIZE+1 : 2*MSIZE); j++) {
            cpar[i][j] = enc_computed[howfar];
            rpar[i][j] = parity[howfar++];
        }
    }
    if (howfar > psize) {
        fprintf(stderr, "Too much data");
    }

    // Fill the matrix with data.
    howfar = 0;
    for (int i = 0; i < MSIZE; i++)
        for (int j = 0; j < MSIZE; j++)
            matrix[i][j] = data[howfar++];

    // Walk through both matrices, check the amount of differences.

    int totaldiff = 0;      // Total difference in all dimensions

    for (int i = 0; i < DIM; i++)
        for (int j = 0; j < ((i < 2) ? MSIZE : 2 * MSIZE - 1); j++)
            // This loop does explicitly NOT include the parity-of-parity bits.
            totaldiff += (cpar[i][j] != rpar[i][j]);

    int result;
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
         * Three/five parity bits have flipped.
         * This can only be caused by 1 data bit flipped, and (if 3) one of the parities involved has also flipped.
         * if 5 (much more likely), one extra parity bit has flipped.
         * First, figure out in which dimension the "extra" parity has flipped, and correct it.
         */
        result = correctExtraParity(totaldiff, cpar, rpar);
        if (result != 0) {
            printf("Could not correct extra parity bit (totaldiff = %d), result = %d\n", totaldiff, result);
        }

        // NO BREAK STATEMENT, use case fallthrough.
    case 4:
        /*
         * One error has occurred. No parities have flipped
         */
        result = correctOne(matrix, cpar, rpar);
        if (result != 0) {
            fprintf(stderr, "Could not correct 1 data error, result=%d\n", result);
            totaldiff = result;
            goto FREE;
        }
        break;

    case 6:
        /*
         * Two errors have occurred. No parity bits flipped, only 2 data bits.
         * Problem is that in 1 dimension, they cancel each other out.
         * So you only have parities in 3 dimensions to figure out what happened.
         */
        result = correctTwoTogether(matrix, cpar, rpar);
        if (result != 0) {
            fprintf(stderr, "Could not correct 2 connected data errors, result = %d\n", result);
            totaldiff = result;
            goto FREE;
        }
        break;

    case 8:
        /*
         * Two errors have occurred. Thankfully, no parity bits have flipped.
         * Furthermore, these errors have occurred in distinct parts of the matrix, dwz
         * no double parities flipped.
         */
        result = correctTwoSeparate(matrix, cpar, rpar);
        if (result != 0) {
            fprintf(stderr, "Could not correct 2 separate data errors, result = %d\n", result);
            totaldiff = result;
            goto FREE;
        }
        break;

    default:
        fprintf(stderr, "Case not handled by any of the other correctors. %d parity bits were different.\n", totaldiff);
        totaldiff = 10001;
        goto FREE;
    }

    // Serialize array back to raw data.
    howfar = 0;
    for (int i = 0; i < MSIZE; i++) {
        for (int j = 0; j < MSIZE; j++) {
            data[howfar++] = matrix[i][j];
        }
    }

    // Free everything, we're not in the Roman empire anymore
    FREE:
    for (int i = 0; i < MSIZE; i++)
        free(matrix[i]);
    free(matrix);
    for (int i = 0; i < DIM; i++) {
        free(cpar[i]);
        free(rpar[i]);
    }
    free(cpar);
    free(rpar);
    free(enc_computed);
    return totaldiff;
}

int correctExtraParity (int totaldiff, bool **cpar, bool **rpar) {
    printf(">>>Correct extra parity bit\n");
    int wrongDim = -1; // What dimension is the dimension in which the other flip happened?
    int countWrongDim=0;
    int whichEl[DIM];
    for (int i = 0; i < DIM; i++) {
        int wrong = 0;
        for (int j = 0; j < ((i < 2) ? MSIZE : 2*MSIZE - 1); j++) {
            if (cpar[i][j] != rpar[i][j]) {     // A flip has occurred
                whichEl[i] = j;
                wrong++;
            }
        }
        if (wrong != 1) {
            wrongDim = i;
            countWrongDim++;
        }
    }
    if (countWrongDim != 1) // Double or zero flip has occurred in more than 1 dimension
        return 1;

    int el;
    switch (wrongDim) {
        case 0: // X direction is wrong
            el = whichEl[2] - whichEl[1];
            break;
        case 1: // Y direction is wrong
            el = whichEl[2] - whichEl[0];
            break;
        case 2: // Diagonal is wrong
            el = whichEl[0] + whichEl[1];
            break;
        case 3: // Cross diagonal
            el = MSIZE - 1 - whichEl[0] + whichEl[1];
            break;
        default:
            return 2;
    }

    // The correct element, the one that had to be flipped in the first place, has been found.
    // Correction must now be done.
    // If totaldiff is 3, then we need to flip the [el] bit in dimension [wrongDim]
    // If totaldiff = 5, we need to find the other bit and flip that.
    if (totaldiff == 3) {
        rpar[wrongDim][el] ^= 1;
    } else if (totaldiff == 5) {
        for (int j = 0; j < ((wrongDim < 2) ? MSIZE : 2 * MSIZE - 1); j++) {
            if ((cpar[wrongDim][j] != rpar[wrongDim][j]) && (j != el)) {
                rpar[wrongDim][j] ^= 1;
            }
        }
    } else {
        return 3;
    }

    return 0;
}

int correctOne(bool **m, bool **cpar, bool **rpar) {
    // Print the function we're in
    printf(">>>Correct 1 data flip:\n");
    // Assumption is made that it is a "nice" error, dwz only 1 data bit has flipped, nothing else.
    int result = 0;

    // First, assert that all parity-of-parities are flipped also, otherwise two errors have occurred.
    // Except if that parity-of-parities is the one that flipped...
    for (int i = 0; i < DIM; i++) {
        int count = 0;
        int tmp = ((i < 2) ? MSIZE : 2 * MSIZE-1);
        if (cpar[i][tmp] == rpar[i][tmp] && count > 1) { // Check final parity bits
            fprintf(stderr, "Parity-of-parities in dimension %d has not flipped\n", i+1);
            return 100+i;
        }
    }

    // Now, allocate memory
    int *whichEl = calloc(DIM, sizeof(int));

    for (int i = 0; i < DIM; i++) {
        for (int j = 0; j < ((i < 2) ? MSIZE : 2 * MSIZE - 1); j++) {
            if (cpar[i][j] != rpar[i][j]) {     // Is there a difference in the parity bits?
                whichEl[i] = j;
            }
        }
    }

    if (whichEl[0] + whichEl[1] != whichEl[2]) {
        fprintf(stderr,"Diagonal check is wrong, at least 3 data bits have flipped.\n");
        result = 200;
        goto FREE;
    }
    if ((MSIZE - 1 - whichEl[0] + whichEl[1]) != whichEl[3]) {
        fprintf(stderr,"Cross diagonal check is wrong, at least 3 data bits have flipped.\n");
        result = 201;
        goto FREE;
    }

    // Everything is correct, flip the bit.
    m[whichEl[0]][whichEl[1]] ^= 1;

    FREE:           // something something slave-joke
    free(whichEl);

    return result;
}

int correctTwoTogether(bool **m, bool **cpar, bool **rpar) {
    // Allocate memory
    int **whereE = calloc(DIM, sizeof (*whereE));
    for (int i = 0; i < DIM; i++)
        whereE[i] = calloc(2, sizeof(*whereE[i]));
    int result = 0;
    int specialDim = -1;

    printf(">>>Correct two connected data flips\n");

    for (int i = 0; i < DIM; i++) {
        int isDiff = 0;
        for (int j = 0; j < ((i < 2) ? MSIZE : 2 * MSIZE - 1); j++) {
            if (cpar[i][j] != rpar[i][j]) {
                whereE[i][isDiff++] = j;
            }
        }

        if (isDiff == 0) {
            specialDim = i;
        } else if (isDiff != 2) {
            fprintf(stderr, "There are not 2 errors in non-special dimension. isDiff = %d, i = %d\n", isDiff, i);
            goto FREE;
        }
    }

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < DIM; j++) {
            printf("%d", whereE[j][i]);
        }
        printf("\n");
    }

    switch (specialDim) { // This is similar to correctTwoSeperate, just with an U for "Zero in this dimension"
    case 0:
        if (MSIZE-1 - whereE[2][0] + 2*whereE[1][0] != whereE[3][0] && MSIZE-1 - whereE[2][0] + 2*whereE[1][0] != whereE[3][1])
            swap(&whereE[2][0], &whereE[2][1]);             // Scenario: (U 0 1 X)
        if (MSIZE-1 - whereE[2][0] + 2*whereE[1][0] == whereE[3][1])
            swap(&whereE[3][0], &whereE[3][1]);             // Scenario: (U 0 0 1)

        for (int i = 0; i < 2; i++)
            whereE[0][i] = whereE[2][i] - whereE[1][i];
        break;

    case 1:
        if (whereE[3][0] + 2*whereE[0][0] - (MSIZE-1) != whereE[2][0] && whereE[3][0] + 2*whereE[0][0] - (MSIZE-1) != whereE[2][1])
            swap(&whereE[2][0], &whereE[2][1]);             // Scenario: (0 U 1 X)
        if (whereE[3][0] + 2*whereE[0][0] - (MSIZE-1) == whereE[2][1])
            swap(&whereE[3][0], &whereE[3][1]);  // Scenario: (0 U 0 1)

        for (int i = 0; i < 2; i++)
            whereE[1][i] = whereE[2][i] - whereE[0][i];
        break;

    case 2:
        if (MSIZE - 1 - whereE[0][0] + whereE[1][0] != whereE[3][0] && MSIZE - 1 - whereE[0][0] + whereE[1][0] != whereE[3][1])
            swap(&whereE[1][0], &whereE[1][1]);             // Scenario: (0 1 U X)
        if (MSIZE - 1 - whereE[0][0] + whereE[1][0] == whereE[3][1])
            swap(&whereE[3][0], &whereE[3][1]);             // Scenario: (0 0 U 1)

        for (int i = 0; i < 2; i++)
            whereE[2][i] = whereE[0][i] + whereE[1][i];

        break;

    case 3:
        if (whereE[0][0] + whereE[1][0] != whereE[2][0] && whereE[0][0] + whereE[1][0] != whereE[2][1])
            swap(&whereE[1][0], &whereE[1][1]);             // Scenario: (0 1 X U)
        if (whereE[0][0] + whereE[1][0] == whereE[2][1])
            swap(&whereE[2][0], &whereE[2][1]);             // Scenario: (0 0 1 U)

        for (int i = 0; i < 2; i++)
            whereE[3][i] = MSIZE - 1 - whereE[0][i] + whereE[1][i];

        break;

    default:
        fprintf(stderr, "There is no special dimension. Abort\n");
        goto FREE;
    }

    for (int i = 0; i < 2; i++) {
        if (whereE[0][i] + whereE[1][i] != whereE[2][i] || whereE[3][i] != whereE[1][i] + MSIZE-1 - whereE[0][i]) {
            fprintf(stderr, "Correction impossible: ");
            for (int j = 0; j < DIM; j++) {
                fprintf(stderr, "%d", whereE[j][i]);
            }
            fprintf(stderr, "\n");
        } else
            m[whereE[0][i]][whereE[1][i]] ^= 1;
    }


    // If all went right, by now the bits are sorted.
//    for (int i = 0; i < 2; i++) {
//        bool err = false;
//        switch (specialDim) {
//        case 0:
//            if (MSIZE-1 - whereE[2][i] + 2*whereE[1][i] != whereE[3][i]) {
//                err=true;
//                break;
//            }
//
//            m[(whereE[2][i] - whereE[1][i])][whereE[1][i]] ^= 1;
//
//            break;
//
//        case 1:
//            if (whereE[3][i] + 2*whereE[0][i] -(MSIZE-1) != whereE[2][i]) {
//                err=true;
//                break;
//            }
//
//            m[whereE[0][i]][(whereE[2][i] - whereE[0][i])] ^= 1;
//
//            break;
//
//        case 2:
//            if (MSIZE-1 - whereE[0][i] + whereE[1][i] != whereE[3][i]) {
//                err = true;
//                break;
//            }
//
//            m[whereE[0][i]][whereE[1][i]] ^= 1;
//
//            break;
//
//        case 3:
//            if (whereE[0][i] + whereE[1][i] != whereE[2][i]) {
//                err = true;
//                break;
//            }
//
//            m[whereE[0][i]][whereE[1][i]] ^= 1;
//
//            break;
//
//        default: break;
//        }
//        if (err) {
//            fprintf(stderr,"Grouping 2 parity bits failed. i = %d, specDim = %d\n", i, specialDim);
//            for (int k = 0; i < 2; i++) {
//                for (int l = 0; l < DIM; l++) {
//                    printf("%d", whereE[l][k]);
//                }
//                printf("\n");
//            }
//            goto FREE;
//        }
//    }

    FREE:
    for (int i = 0; i < DIM; i++)
        free(whereE[i]);
    free(whereE);

    return result;
}


int correctTwoSeparate(bool **m, bool **cpar, bool **rpar) {
    // Allocate memory
    int **whereE = calloc(DIM, sizeof(*whereE));
    for (int i = 0; i < 4; i++)
        whereE[i] = calloc(2, sizeof(*whereE[i]));
    int result = 0;

    printf(">>Correct 2 separate data flips:\n");

    for (int i = 0; i < DIM; i++) {
        int k = 0; // How much'th bit
        for (int j = 0; j < ((i < 2) ? MSIZE : 2 * MSIZE - 1); j++) {
            if (cpar[i][j] != rpar[i][j]) {
                whereE[i][k++] = j;
            }
        }
        if (k != 2) {// This should not be possble, because only 2 errors per dimension. If everything is correct.
            fprintf(stderr,"GEKKE DINGEN\n");
            result = 2;
            goto FREE;
        }
    }
    // whereE contains  the location of all flipped parity bits.
    // There are 16 possible scenario's, of which 8 are relevant. Those 8 flipped parity bits correspond to
    // 2 flipped data bits, which means that 4 parities belong to 1 data bit. The trick is figuring out which.
    // The ordering is done by assuming the first one is correct (whereE[0][0]), and figuring the rest out.
    // Because this assumption can be safely made, only half the possible scenario's are relevant.
    // Because we don't care which data bit is "first", only that all parity bits belong to the correct data bit.
    // Thus, we look at 8 scenario's, given the different states of all bits. 0 means that they are corresponding to the first
    // bit, X means unknown, and 1 means the parity bits need to be flipped.

    if (whereE[0][0] + whereE[1][0] != whereE[2][0] && whereE[0][0] + whereE[1][0] != whereE[2][1])
        swap(&whereE[1][0], &whereE[1][1]);        // Scenario: (0 1 X X)

    if (whereE[0][0] + whereE[1][0] == whereE[2][0]) { // Scenario: 0 0 0 X
        // First three are correct
        if (MSIZE - 1 - whereE[0][0] + whereE[1][0] == whereE[3][1]) { // Scenario: 0 0 0 1
            // Only fourth one is incorrect
            swap(&whereE[3][0], &whereE[3][1]);
        } else if (MSIZE - 1 - whereE[0][0] + whereE[1][0] != whereE[3][0]) { // Scenario non-conform.
            // Something fucked up tremendously, fourth one is STILL incorrect
            fprintf(stderr, "Something went horribly wrong\n");
            result = 3;
            goto FREE;
        }
    } else if (whereE[0][0] + whereE[1][0] == whereE[2][1]) { // Third one is incorrect (0 0 1 X)
        swap(&whereE[2][0], &whereE[2][1]);

        // Check the fourth one
        if (MSIZE - 1 - whereE[0][0] + whereE[1][0] == whereE[3][1]) // Scenario (0 0 0 1)
            swap(&whereE[3][0], &whereE[3][1]);
        else if (MSIZE - 1 - whereE[0][0] + whereE[1][0] != whereE[3][0]) { // Scenario: (0 0 0 NON-CONFORM)
            fprintf(stderr, "Something went horribly wrong\n");
            result = 4;
            goto FREE;
        }
    } else {
        // Completely broken
        fprintf(stderr, "Something went horribly wrong\n");
        result = 5;
        goto FREE;
    }

    // Final assertion:
    for (int i = 0; i < 2; i++) {
        bool isDiagCorrect = (whereE[0][i] + whereE[1][i] == whereE[2][i]);
        bool isCrossCorrect = (MSIZE - 1 - whereE[0][i] + whereE[1][i] == whereE[3][i]);
        if (!(isDiagCorrect && isCrossCorrect)) {
            result = 6; // Clearly the 'fixing' went wrong.
            goto FREE;
        }
    }

    for (int i = 0; i < 2; i++) { // The actual flipping.
        m[whereE[0][i]][whereE[1][i]] ^= 1;
    }

    FREE:
    for (int i = 0; i < DIM; i++) {
        free(whereE[i]);
    }
    free(whereE);
    return result;
}

void swap (int *a, int *b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}