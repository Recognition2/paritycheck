//
// Created by Gregory Hill on 9-12-16.
//

// Include defined libraries
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>        // Boolean values
#include <math.h>

// Include own files
#include "enc.h"
#include "dec.h"

// Declare functions used
void printhelp(char *filename);                 // How to use function, and what went wrong?
bool *readHexFromFile (char *filename);         // Return bits for usage
void writeHexToSTDOUT (bool *data);             // Write encoded bits


int main(int argc, char *argv[]) {
//    const int dsize = (int) pow(MSIZE,DIM);						// in bits, size the data block
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
    data = readHexFromFile(argv[1]);

	// At this point, data is a vector containing boolean representation of the data.
	encoded = enc(data);
	// The encoded data contains ONLY the parity bits.

    /////
    // Now do strange things with the data, like ABSOLUTELY DESTROYING IT
    /////


    data = dec(data, encoded);
	// Data is only needed for calculation of the parity bits

    writeHexToSTDOUT(encoded);

    free(data);
    free(encoded);
}

bool *readHexFromFile (char *filename) {
    const int datasize = (int) pow(MSIZE,DIM);						// in bits, size the data block
    int bitfactor = 4;				// Because hexadecimal
    char *buff = calloc((size_t) datasize/bitfactor + (datasize%bitfactor != 0), sizeof(int));	// 1 hex char = 4 bits #HEXHARDCODE
    bool *data = calloc ((size_t) datasize, sizeof(bool));			// Data to be encoded
    //TODO: Fix hardcoded dependency on hex data format

    FILE *fp;   // File pointer

    fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr,"File does not exist!\n");
        exit(1);
    }

    // Amount to read is always equal to size^dim
    if (fgets(buff, datasize/bitfactor+1, fp) == NULL) {
        fprintf(stderr,"Error reading from file");
        exit(1);
    }
    fclose(fp);     // Reading complete, file not necessary anymore.

    // Decode read data
    for (int i = 0; i < datasize/bitfactor; i ++) {
        int t = (int) buff[i];	// tmp cast
        u_int8_t mask = 0x1;
        if (t >= 47 && t <= 57) { 	// Character is 0-9, in hexadecimal
            t -= 46;
        } else if (t >= 97 && t <= 102) { // Character is a-f in hexadecimal
            t -= 96;
        } else if (t == 0) {		// EOF
            break;
        } else { // Character is not in either range, so probably an Evil character. Just skip it.
            fprintf(stderr,"This character is clearly not hexadecimal, go eat a bag of %d!", t);
            exit(EXIT_FAILURE);
        }

        for (int j = 0; j < bitfactor; j++) {
            data[i * bitfactor + j] = (bool) (t & (mask << j));
        }
    }
    free(buff);	// File reading buffer no longer needed
    // At this point, data is a vector containing boolean representation of the data.
    return data;
}

void writeHexToSTDOUT (bool *data) {
    ////////////////////////////////
    // Decode the returned vector, and write to STDOUT
    ////////////////////////////////
    const int datasize = (int) pow(MSIZE,DIM);						// in bits, size the data block
    const int blocksize = datasize + (MSIZE * DIM) + 1;				// in bits, amount of data in 1 block
    const int psize = blocksize - datasize;
    int bitfactor = 4;				// Because hexadecimal

    // The amount of characters that needs to be reserved.
    // In case of a nonzero modulo, one more bit needs to be allocated.
    int charcount = psize/bitfactor + (psize%bitfactor != 0);
    char *enchex = calloc((size_t) charcount, sizeof(char));

    static char hexconv[] = "0123456789abcdef";			// The character set corresponding to hexadecimal encoding

    for (int i = 0; i < charcount; i++) {
        // Convert all characters
        int value = 0;
        for (int j = 0; j < bitfactor; j++) {
            if (bitfactor*i + j > psize - 1) {
                break;
            }
            value += (data[bitfactor*i + j] << (bitfactor - j -1));
            // Shift the binary value to the left, equal to at most 3 shifts.
        }
        if (value > 15) {
            fprintf(stderr, "\nDecoded data larger than 15 (%d), must not happen\n", value);
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