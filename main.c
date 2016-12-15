//
// Created by Gregory Hill on 9-12-16.
//

#include <stdio.h>
#include <memory.h>         // For strcmp
#include <stdbool.h>        // Boolean values
#include "enc.h"

#define DIM 4
#define MATRIXSIZE 6

void printhelp(char *filename) {
    printf("This program simulates a four dimensional parity check on data. It can both encode and decode data.\n"
    "Encoding can be done by calling '%s enc <data>', dec similarly.\n"
    "The data is (not yet) written to <filename>.enc in case of encode, or <filename>.dec in case of decoding.\n"
    "Furthermore, it was fun to make this program.", filename);
    return;
}


int main(int argc, char *argv[]) {
    // Main function

    // First, check number of arguments
    if (argc < 2) {
        printhelp(argv[0]);
        return 1;
    }

    // From here on, the first argument needs to be parsed. Based on that, the rest can be parsed too.
    if (strcmp(argv[1], "help") == 0) {
        printhelp(argv[0]);
        return 1;
    } else if ((strcmp(argv[1], "enc") == 0) || (strcmp(argv[1], "encode"))) {
        enc(argv[2])
    } else if ((strcmp(argv[1], "dec") == 0) || (strcmp(argv[1], "decode"))) {
        //decode();
    }
    return 0;
}