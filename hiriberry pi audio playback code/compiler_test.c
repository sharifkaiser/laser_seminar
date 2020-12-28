// After installing bcm2835, you can build this
// with something like:
// gcc -o compiler_test compiler_test.c -l bcm2835 -l m	 /* output in compiler_test file, using library bcm2835 and m for math */
// to run: sudo ./compiler_test
// link for gcc arguments https://gcc.gnu.org/onlinedocs/gcc/Overall-Options.html#Overall-Options

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>
#include <bcm2835.h>
#include <unistd.h>

int main(int argc, char** argv)
{
	printf("size of int: %d", sizeof(int));
}
