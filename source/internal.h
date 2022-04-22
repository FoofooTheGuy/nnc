
#ifndef inc_internal_h
#define inc_internal_h

#include <nnc/base.h>

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

#define u8 nnc_u8
#define u16 nnc_u16
#define u32 nnc_u32
#define u64 nnc_u64

#define i8 nnc_i8
#define i16 nnc_i16
#define i32 nnc_i32
#define i64 nnc_i64

#define result nnc_result

#define bswap16 nnc_bswap16
#define bswap32 nnc_bswap32
#define bswap64 nnc_bswap64

#define rstream nnc_rstream

#define BE16(a) (bswap16(a))
#define BE32(a) (bswap32(a))
#define BE64(a) (bswap64(a))

#define LE16(a) (a)
#define LE32(a) (a)
#define LE64(a) (a)

#define U16P(a) (* (u16 *) (a))
#define U32P(a) (* (u32 *) (a))
#define U64P(a) (* (u64 *) (a))

#define BE16P(a) BE16(U16P(a))
#define BE32P(a) BE32(U32P(a))
#define BE64P(a) BE64(U64P(a))

#define LE16P(a) LE16(U16P(a))
#define LE32P(a) LE32(U32P(a))
#define LE64P(a) LE64(U64P(a))

#define TRYB(expr) if(( expr ) != NNC_R_OK) return false
#define TRY(expr) if((ret = ( expr )) != NNC_R_OK) return ret

nnc_u16 nnc_bswap16(nnc_u16 a);
nnc_u32 nnc_bswap32(nnc_u32 a);
nnc_u64 nnc_bswap64(nnc_u64 a);

typedef struct nnc_rstream nnc_rstream;
#define read_at_exact nnc_read_at_exact
result nnc_read_at_exact(nnc_rstream *rs, u32 offset, u8 *data, u32 dsize);
#define read_exact nnc_read_exact
result nnc_read_exact(nnc_rstream *rs, u8 *data, u32 dsize);

#endif

