//
// Created by Gregory Hill on 9-12-16.
//

// Include defined libraries
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>        // Boolean values
#include <memory.h>
#include <math.h>			// Pow function only

// Include own files
#include "enc.h"

void printhelp(char *filename) {    // Help function
    printf("This program simulates a four dimensional parity check on data. It can both encode and decode data.\n"
    "Encoding can be done by calling '%s enc <data>', dec similarly.\n"
    "The data is (not yet) written to <filename>.enc in case of encode, or <filename>.dec in case of decoding.\n"
    "Furthermore, it was fun to make this program.", filename);
    return;
}

int main(int argc, char *argv[]) {
	// First, check whether the block size is larger than would fit into 4 bytes
	if (powl((long double)MSIZE, (long double) DIM) > 4294967295) {
		fprintf(stderr, "Maximum block size has been overridden!\n");
		exit(-1);
	}
    // Variable initialization
	FILE *fp;
	int datasize = (int) pow(MSIZE,DIM);						// in bits, size the data block
	int blocksize = datasize + (MSIZE * DIM) + 1;				// in bits, amount of data in 1 block
	int bitfactor = 4;				// Because hexadecimal
	char *buff = calloc((size_t) datasize/bitfactor, sizeof(int));	// 1 hex char = 4 bits #HEXHARDCODE TODO: Fix hardcoded hex dep
	bool *data = calloc ((size_t) datasize, sizeof(bool));			// Data to be encoded
	//TODO: Fix hardcoded dependency on hex data format

    // First, check number of arguments
    if (argc < 1) {
        printhelp(argv[0]);
        return 1;
    }

	// Assumption is made that the (only) argument is the filename containing data to encode
	fp = fopen(argv[1], "r");

	// Amount to read is always equal to size^dim
	if (fgets(buff, datasize/bitfactor+1, fp) == NULL) {
		printf("Error reading from file");
		return -1;
	}
	fclose(fp);
	for (int i = 0; i < datasize/bitfactor; i ++) {
		int t = (int) buff[i];	// tmp cast
		u_int8_t mask = 0x1;
		if (t >= 47 && t <= 57) { 	// Character is 0-9, in hexadecimal
			t -= 46;
		} else if (t >= 97 && t <= 102) { // Character is a-f in hexadecimal
			t -= 96;
		} else if (t == 0) {		// EOF
			break;
		} else { // Character is not in either range, so probably an Evil character
			printf("This character is clearly not hexadecimal, go eat a bag of %d!", t);
			return -1;
		}

		for (int j = 0; j < bitfactor; j++) {
			data[i * bitfactor + j] = (bool) (t & (mask << j));
		}
	}
	printf("Dit werkt nog!\n");

	// At this point, data is a vector containing boolean representation of the data.
	bool *encoded = enc(data);


	// Decode the data..
	char *enchex = calloc((size_t) (blocksize - datasize)/bitfactor, sizeof(char));
	for (int i = 0; i < blocksize/bitfactor; i ++) {
		static char hexconv[] = "0123456789abcdef";
		int value = 0;
		for (int j = 0; j < bitfactor; j++) {
			value += encoded[bitfactor*i + j] << j;
		}
		enchex[i] = hexconv[value];
	}

	printf("%s", enchex);
	free(encoded);
	free(buff);
    return 0;
}