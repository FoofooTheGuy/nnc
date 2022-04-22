
#include <nnc/u128.h>
#include <stdlib.h>
#include <stdio.h>
#include "./internal.h"


void nnc_u128_xor(u128 *a, const u128 *b)
{
	a->hi ^= b->hi;
	a->lo ^= b->lo;
}

void nnc_u128_rol(u128 *a, u8 n)
{
	if(n < 64)
	{
		u64 saved_hi = (a->hi & (UINT64_MAX << (64 - n))) >> (64 - n);
		u64 saved_lo = (a->lo & (UINT64_MAX << (64 - n))) >> (64 - n);
		a->hi <<= n;
		a->lo <<= n;
		a->hi |= saved_lo;
		a->lo |= saved_hi;
	}
	else
	{
		fprintf(stderr, "EINVAL: rol: n > 64 (n=%i)\n", n);
		abort();
	}
}

void nnc_u128_ror(u128 *a, u8 n)
{
	if(n < 64)
	{
		u64 saved_hi = (a->hi & ~(UINT64_MAX << n)) << (64 - n);
		u64 saved_lo = (a->lo & ~(UINT64_MAX << n)) << (64 - n);
		a->hi >>= n;
		a->lo >>= n;
		a->hi |= saved_lo;
		a->lo |= saved_hi;
	}
	else
	{
		fprintf(stderr, "EINVAL: ror: n > 64 (n=%i)\n", n);
		abort();
	}
}

void nnc_u128_add(u128 *a, const u128 *b)
{
	u64 carry;
	u64 add = a->lo + b->lo;
	/* overflow */
	if(add < a->lo)
	{
		a->lo = UINT64_MAX;
		carry = add;
	}
	else
	{
		a->lo = add;
		carry = 0;
	}
	a->hi += b->hi + carry;
}

void nnc_u128_bytes(const u128 *a, u8 bytes[0x10])
{
	u64 *bytes64 = (u64 *) bytes;
	bytes64[0] = BE64(a->hi);
	bytes64[1] = BE64(a->lo);
}

