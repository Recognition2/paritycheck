## Forward Error correction
####Implementing multi dimensional parity check code, to target embedded hardware and PocketQubes.

This code uses four dimensional parity to enable double error correction.
In some cases, even triple errors can be corrected, however this is not always the case.
The implemented code is based on [Bilal, Khan & Khan's parity based error correction algorithm](http://eecs.ucf.edu/~skhan/Publications/Download/YBilal-2013-ICOSST.pdf)

The focus lies on low computational complexity, such that even a CubeSat or PocketQube can implement this code.
Furthermore, block size needs to be low, such that one whole block can be loaded into memory for checking.
Just as important is a low overhead, it's pretty useless if half your RAM is eaten by this parity check.

All these parameters depend only on the size of the (square) matrix that is used.
This `MSIZE` can be changed in the [`globals.h`](src/globals.h) file.
For testing, a small size is useful (like 4), because then the code can be inspected manually.

For production, it depends on your own needs.
A sensible starting point is 128.
This produces a block size of 2 kilobyte (128*128 = 16384 bits)
The chance of a double error occurring in space is equal to 10<sup>-18</sup>.
It has an overhead of 5\%. 

Encoding is performed in the [`src/enc.c`](src/enc.c) file.
* This function first creates a matrix from the data. 
* Then the parity checks in all dimensions are executed.
* After this, the final "parity of parity" bits are calculated.
* Then, this ragged array of bits is serialized, and a pointer to this bitstring is returned to the calling function.

Decoding is performed in [`src/dec.c`](src/dec.c). 
* This function takes both the data (possibly corrupt) and the parity bits (also possibly corrupt) as arguments.
* It then attempts to identify all possible scenario's where 1 or 2 bits have flipped (in either data or parity bits, or a combination).
* It does this by calculating the parity bits, and comparing this with the given parities.

### Speedtest
The speedtest works quite simply: 
* First it reads a whole block of data into memory (to make sure disk access times are not counted).
* Then the encoding operation is performed, and the result printed.
* The time it took to do this is printed as well.
* The data is corrupted
* This corrupted data is fed to the decoding function
* Print timing results

The counter can be increased manually, by default is tests 10000 blocks, or 20 MB of data.


### Usage
The `enc` function, encodes data and returns the parity bits. 

The `dec` function takes the array of data and parity bits, and modifies this raw data when applying correction. 
It returns the amount of errors that happened.

### Building
When CLion creates a project, everything is configured automatically.
This is the recommended and easiest setup.

Manual compilation has only been tested on Linux.

1) Install `cmake`,`make` and `git`.

Example using Ubuntu:
```bash
$ sudo apt-get install cmake make git
```

Using Archlinux:
```bash
$ sudo pacman -S make cmake git --needed
```

2) Clone the repository:
```
git clone https://github.com/Recognition2/paritycheck
```

3) Create a build directory in the root directory, cd to it, and execute cmake and make:
```
$ cd paritycheck/
$ mkdir cmake-build-debug
$ cd cmake-build-debug

$ cmake ..
$ make
```

The binary is now available in `cmake-build-debug/fourDimParity`.

It might be sensible to generate some testing data, this can be done using OpenSSL:
```
openssl rand -hex $(echo "1024*1024*24" | bc) > testdata.txt
```