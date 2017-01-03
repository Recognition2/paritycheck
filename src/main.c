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
bool *readHexFromFile (char *filename, int amount);         // Return bits for usage
void writeHexToSTDOUT (bool *data, int amount);             // Write encoded bits

int main(int argc, char *argv[]) {
    const int dsize = (int) pow(MSIZE,DIM);						// in bits, size the data block
//    const int bsize = dsize + (MSIZE * DIM) + 1;				// in bits, amount of data in 1 block
//    const int psize = bsize - dsize;
    bool *encoded = NULL, *data = NULL;

    // First, check whether the block size is larger than would fit into 4 bytes
	if (powl((long double)MSIZE, (long double) DIM) > 4294967295) {
		fprintf(stderr, "Maximum block size has been overridden!\n");
		exit(1);
	}

    // Check number of arguments
    if (argc < 1) {
        printhelp(argv[0]);
        return 1;
    }

	// Assumption is made that the (only) argument is the filename containing data to encode
    data = readHexFromFile(argv[1], 100*dsize);

    struct timespec ts1, ts2;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts1);
    for (int i = 0; i < 100; i++) {
        // At this point, data is a vector containing boolean representation of the data.
        encoded = enc(data+dsize*i);
    }
	// The encoded data contains ONLY the parity bits.
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts2);
    double posix_dur = 1000.0*ts2.tv_sec + 1e-6*ts2.tv_nsec - (1000.0*ts1.tv_sec + 1e-6*ts1.tv_nsec);
    printf("Time difference: %f seconds\n",posix_dur);
    // Now do strange things with the data, like ABSOLUTELY DESTROYING IT

    int errors = dec(data, encoded);
    if (errors == 10000) {
        // Something went horribly wrong, data could not be decoded
        fprintf(stderr, "HALP\n");
    }


    printf("\n%d errors occurred\n", errors);

    writeHexToSTDOUT(data, dsize);

    free(data);
    free(encoded);
}

bool *readHexFromFile (char *filename, int amount) {
    int bitfactor = 4;				// Because hexadecimal
    bool *data = calloc ((size_t) amount, sizeof(bool));			// Data to be encoded

    FILE *fp;   // File pointer
    fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "File does not exist!\n");
        exit(EXIT_FAILURE);
    }

    // Read & decode datas
    int i = 0;
    while (i < amount/bitfactor-1) {
        int t = fgetc(fp);
        u_int8_t mask = 0x1;

        if (t >= 48 &&  t <= 57) { 	// Character is 0-9, in hexadecimal
            t -= 48;
            i++;
        } else if (t >= 97 && t <= 102) { // Character is a-f in hexadecimal
            t -= (97-10);
            i++;
        } else if (feof(fp) != 0) { // EOF has occurred
            fprintf(stderr,"End of file was reached before all necessary characters could be read out.");
            exit(EXIT_FAILURE);
        }

        for (int j = 0; j < bitfactor; j++) {
            data[i * bitfactor + j] = (bool) (t & (mask << (bitfactor - j - 1)));
        }
    }
    fclose(fp);     // Reading complete, file not necessary anymore.

    // At this point, data is a vector containing boolean representation of the data.
    return data;
}

void writeHexToSTDOUT (bool *data, int amount) {
    // Decode the returned vector, and write to STDOUT
    int bitfactor = 4;				// Because hexadecimal

    // The amount of characters that needs to be reserved.
    // In case of a nonzero modulo, one more bit needs to be allocated.
    int charcount = amount/bitfactor + (amount%bitfactor != 0);
    char *enchex = calloc((size_t) charcount, sizeof(char));

    static char hexconv[] = "0123456789abcdef";			// The character set corresponding to hexadecimal encoding
    bool flag = true;
    for (int i = 0; i < charcount && flag; i++) {
        short value = 0;
        bool *tmp = data + i * bitfactor * sizeof(bool);
        for (int j = 0; j < bitfactor; j++) {
            if (i*bitfactor > amount) {
                flag = false;
                break;
            }
            value += tmp[j] << (bitfactor - j - 1);
        }
        enchex[i] = hexconv[value];
    }

    // Encoded data has been converted

    // Print the encoded parity bits
    for (int i = 0; i < charcount; i++ ){
        printf("%c", enchex[i]);
    }

    // Characters have been printed
    free(enchex);
    return;
}

void printhelp(char *filename) {    // Help function
    printf("This program simulates a four dimensional parity check on data. It can both encode and decode data.\n"
                   "Encoding can be done by calling '%s enc <data>', dec similarly.\n"
                   "The data is (not yet) written to <filename>.enc in case of encode, or <filename>.dec in case of decoding.\n"
                   "Furthermore, it was fun to make this program.", filename);
    return;
}
