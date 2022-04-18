// vim: ft=c.doxygen
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

typedef uint8_t nnc_u8;
typedef uint16_t nnc_u16;
typedef uint32_t nnc_u32;
typedef uint64_t nnc_u64;

typedef int8_t nnc_i8;
typedef int16_t nnc_i16;
typedef int32_t nnc_i32;
typedef int64_t nnc_i64;

typedef nnc_u32 nnc_result;

enum nnc_result_codes {
	NNC_R_OK = 0,     /** Everything went fine. */
	NNC_R_FAIL_OPEN,  /** Failed to open. */
	NNC_R_SEEK_RANGE, /** Invalid seek range. */
	NNC_R_FAIL_READ,  /** Failed to read. */
	NNC_R_TOO_SMALL,  /** Reading yielded too little data. */
	NNC_R_TOO_LARGE,  /** Input data too large. */
};


nnc_u16 nnc_bswap16(nnc_u16 a);
nnc_u32 nnc_bswap32(nnc_u32 a);
nnc_u64 nnc_bswap64(nnc_u64 a);

NNC_END
#endif

