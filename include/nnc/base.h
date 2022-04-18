// vim: ft=c.doxygen
/** \file  base.h
 *  \brief Basic type definitions.
 */
#ifndef inc_nnc_base_h
#define inc_nnc_base_h

#include <stdbool.h>

#ifdef __cplusplus
	#define NNC_START  extern "C" {
	#define NNC_END    }
#else
	#define NNC_START
	#define NNC_END
#endif

#include <stdint.h>
#include <stddef.h>
NNC_START

typedef uint8_t nnc_u8;   ///< 8-bit unsigned integer.
typedef uint16_t nnc_u16; ///< 16-bit unsigned integer.
typedef uint32_t nnc_u32; ///< 32-bit unsigned integer.
typedef uint64_t nnc_u64; ///< 64-bit unsigned integer.

typedef int8_t nnc_i8;   ///< 8-bit signed integer.
typedef int16_t nnc_i16; ///< 16-bit signed integer.
typedef int32_t nnc_i32; ///< 32-bit signed integer.
typedef int64_t nnc_i64; ///< 64-bit signed integer.

typedef nnc_u32 nnc_result; ///< Result type.

enum nnc_result_codes {
	NNC_R_OK,         ///< Everything went fine.
	NNC_R_FAIL_OPEN,  ///< Failed to open.
	NNC_R_SEEK_RANGE, ///< Invalid seek range.
	NNC_R_FAIL_READ,  ///< Failed to read.
	NNC_R_TOO_SMALL,  ///< Reading yielded too little data.
	NNC_R_TOO_LARGE,  ///< Input data too large.
};


NNC_END
#endif

