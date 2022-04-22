
#include <nnc/u128.h>
#include <stdio.h>


int u128_main(int argc, char *argv[])
{
	// TODO: Write proper test
	(void) argc;
	(void) argv;
	nnc_u128 a = NNC_HEX128(0x1111111111111111,5555555555555555);
	nnc_u128 b = NNC_HEX128(0x1111111111111111,5555555555555555);
	printf("0x%016lX%016lX + 0x%016lX%016lX = ", a.hi, a.lo, b.hi, b.lo);
	nnc_u128_add(&a, &b);
	printf("0x%016lX%016lX\n", a.hi, a.lo);
	return 0;
}

