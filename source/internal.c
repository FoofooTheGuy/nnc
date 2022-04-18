
#include <nnc/base.h>
#include "./internal.h"
#include <nnc/read-stream.h>


static void bswap(u8 *r, u8 *n, u8 len)
{
	for(u8 i = 0; i < len; ++i)
		r[i] = n[len - i - 1];
}

#define MKBSWAP(n) \
	uint##n##_t nnc_bswap##n(u##n a) \
	{ u##n r; bswap((u8 *) &r, (u8 *) &a, n/8); return r; }

MKBSWAP(16)
MKBSWAP(32)
MKBSWAP(64)

/* also contains implementations from in internal.h */

result nnc_read_at_exact(nnc_rstream *rs, u32 offset, u8 *data, u32 dsize)
{
	result ret;
	u32 size;
	TRY(NNC_RS_PCALL(rs, seek_abs, offset));
	TRY(NNC_RS_PCALL(rs, read, data, dsize, &size));
	return size == dsize ? NNC_R_OK : NNC_R_TOO_SMALL;
}

result nnc_read_exact(nnc_rstream *rs, u8 *data, u32 dsize)
{
	result ret;
	u32 size;
	TRY(NNC_RS_PCALL(rs, read, data, dsize, &size));
	return size == dsize ? NNC_R_OK : NNC_R_TOO_SMALL;
}

