// vim: ft=c.doxygen
/** \file   u128.h
 *  \brief  An simple 128-bit unsigned integer implementation.
 */
#ifndef inc_nnc_u128_h
#define inc_nnc_u128_h

#include <nnc/base.h>
NNC_START

typedef struct nnc_u128 {
	nnc_u64 hi;
	nnc_u64 lo;
} nnc_u128;

/** NNC_HEX128(0x1234567887654321,1234567887654321) */
#define NNC_HEX128(hi, lo) \
	((nnc_u128) { hi, 0x##lo })

/** Xor a with b saving the result in a and returning the pointer. */
nnc_u128 *nnc_u128_xor(nnc_u128 *a, const nnc_u128 *b);
/** Add a with b saving the result in a and returning the pointer. */
nnc_u128 *nnc_u128_add(nnc_u128 *a, const nnc_u128 *b);
/** Rotate a n bits to the right saving the result in a and returning the pointer, n is at max 63. */
nnc_u128 *nnc_u128_rol(nnc_u128 *a, nnc_u8 n);
/** Rotate a n bits to the left saving the result in a and returning the pointer, n is at max 63. */
nnc_u128 *nnc_u128_ror(nnc_u128 *a, nnc_u8 n);
/** Export the u128 as big-endian bytes */
void nnc_u128_bytes(const nnc_u128 *a, nnc_u8 bytes[0x10]);

NNC_END
#endif

