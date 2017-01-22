//
// Created by Gregory Hill on 9-12-16.
//

// Include defined libraries
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>        // Boolean values
#include <math.h>
#include <time.h>

// Include own files
#include "enc.h"
#include "dec.h"

// Declare functions used
void printhelp(char *filename);                 // How to use function, and what went wrong?
bool *readHexFromFile (FILE *fp, int amount);         // Return bits for usage
void writeHexToSTDOUT (bool *data, int amount);             // Write encoded bits
long double convertTime(struct timespec a, struct timespec b);

int main(int argc, char *argv[]) {
    const int dsize = MSIZE*MSIZE;            // Data block size
    const int psize = 6*MSIZE - 2;            // Parity size
//    const int bsize = dsize + psize;          // Total block size

    // Initialize timers
    struct timespec zero, readDone, encDone, decDone;

    // First, check whether the block size is larger than would fit into 4 bytes
	if (powl((long double)MSIZE, 2) > 4294967295) {
		fprintf(stderr, "Maximum block size has been overridden!\n");
		exit(1);
	}

    // Check number of arguments
    if (argc < 1) {
        printhelp(argv[0]);
        return 1;
    }

    // Open file
    FILE *fp;
    fp = fopen(argv[1], "r");

    if (fp == NULL) {
        fprintf(stderr, "File does not exist!\n");
        exit(EXIT_FAILURE);
    }

    // Encode n blocks
    int n = 10000;
    bool **raw = calloc((size_t) n, sizeof(bool*)); // Allocate storage for all raw data bits
    bool **encoded = calloc((size_t) n, sizeof(bool*));

    bool **old = calloc((size_t) n, sizeof(bool *));  // Save old binary data here
    for (int i = 0; i < n; i++)
        old[i] = calloc(dsize, sizeof(bool));

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &zero);

    for (int i = 0; i < n; i++) { // Reading must happen between fopen and fclose calls
        raw[i] = readHexFromFile(fp, dsize);
        for (int j = 0; j < dsize; j++) {
            old[i][j] = raw[i][j];
        }
    }

    fclose(fp); // Reading complete

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &readDone);

    // At this point, data is a vector containing boolean representation of the data.
    for (int i = 0; i < n; i++) {
        encoded[i] = enc(raw[i]);
    }

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &encDone);

    // The encoded data contains ONLY the parity bits.
    // Now do strange things with the data, like ABSOLUTELY DESTROYING IT

    //////////////////// DESTRUCTION ///////////////////////////////
    for (int i = 0; i < n; i++) {
        raw[i][(i+4)%dsize] ^= 1;
        raw[i][(i+i+2)%dsize] ^= 1;
    }

    for (int i = 0; i < n; i++)
        dec(raw[i], encoded[i]);

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &decDone);

    for (int i = 0; i < n; i++) {
        int wrong = 0;
        for (int j = 0; j < dsize; j++) {
            if (old[i][j] != raw[i][j]) {
                fprintf(stderr, "Data correction went wrong: %d'th attempt, bit %d\n", i, j);
                wrong++;
            }
        }
        if (wrong != 0) {
            printf("!How unfortunate! Correct data: \t");
            writeHexToSTDOUT(old[i], dsize);
            printf("!Wrongly corrected to:\t");
            writeHexToSTDOUT(raw[i], dsize);
        }
    }

    writeHexToSTDOUT(encoded[0], psize);

    // Print timings;
    printf(">Reading took %Le seconds\n", convertTime(readDone, zero));
    printf(">Encoding took %Le seconds\n", convertTime(encDone, readDone));
    printf(">Decoding took %Le seconds\n", convertTime(decDone, encDone));

    for (int i = 0; i < n; i++) {
        free(raw[i]);
        free(encoded[i]);
        free(old[i]);
    }
    free(raw);
    free(encoded);
    free(old);
}

bool *readHexFromFile (FILE *fp, int amount) {
    int bitfactor = 4;				// Because hexadecimal
    bool *data = calloc ((size_t) amount, sizeof(bool));			// Data to be encoded

    // Read & decode datas
    int i = 0;
    while (i < amount/bitfactor+1) {
        int t = fgetc(fp);

        if (t >= 48 &&  t <= 57) { 	// Character is 0-9, in hexadecimal
            t -= 48;
        } else if (t >= 97 && t <= 102) { // Character is a-f in hexadecimal
            t -= (97-10);
        } else if (feof(fp) != 0) { // EOF has occurred
            fprintf(stderr,"End of file was reached before all necessary characters could be read out (%d/%d).", t, amount/bitfactor);
            exit(EXIT_FAILURE);
        }

        for (int j = 0; (j < bitfactor) && ((i*bitfactor+j) < amount); j++) {
            data[i * bitfactor + j] = (bool) (t & (0x1 << (bitfactor - j - 1)));
        }
        i++;
    }

    // At this point, data is a vector containing boolean representation of the data.
    return data;
}

void writeHexToSTDOUT (bool *data, int amount) {
    // Decode the returned vector, and write to STDOUT
    int bitfactor = 4;				// Because hexadecimal

    // The amount of characters that needs to be reserved.
    // In case of a nonzero modulo, one more bit needs to be allocated.
    int charcount = amount/bitfactor + (amount%bitfactor != 0);
    char *enchex = calloc((size_t) charcount, sizeof(*enchex));

    static char hexconv[] = "0123456789abcdef";			// The character set corresponding to hexadecimal encoding
    bool flag = true;
    for (int i = 0; i < charcount && flag; i++) {
        short value = 0;
        bool *tmp = data + i * bitfactor * sizeof(*tmp);
        for (int j = 0; j < bitfactor; j++) {
            if (i*bitfactor+j >= amount) {
                flag = false;
                break;
            }
            value += tmp[j] << (bitfactor - j - 1);
        }
        enchex[i] = hexconv[value];
    }

    // Encoded data has been converted

    // Print the encoded bits
    for (int i = 0; i < charcount; i++ ){
        printf("%c", enchex[i]);
    }
    printf("\n");

    // Characters have been printed
        free(enchex);
    return;
}

long double convertTime(struct timespec a, struct timespec b) {
    return 1e-9*(a.tv_nsec - b.tv_nsec) + difftime(a.tv_sec, b.tv_sec);
}

void printhelp(char *filename) {    // Help function
    printf("This program simulates a four dimensional parity check on data. It can both encode and decode data.\n"
                   "Encoding can be done by calling '%s <data>file', dec similarly.\n"
                   "The data is written to stdout.\n", filename);
    return;
}
