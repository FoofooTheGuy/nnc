
#include <nnc/base.h>
#include <nnc/read-stream.h>
#include <ctype.h>
#include "./internal.h"


#ifdef __GNUC__
#define MKBSWAP(n) \
	u##n nnc_bswap##n(u##n a) \
	{ return __builtin_bswap##n(a); }
#else
static void bswap(u8 *r, u8 *n, u8 len)
{
	for(u8 i = 0; i < len; ++i)
		r[i] = n[len - i - 1];
}

#define MKBSWAP(n) \
	u##n nnc_bswap##n(u##n a) \
	{ u##n r; bswap((u8 *) &r, (u8 *) &a, n/8); return r; }
#endif

MKBSWAP(16)
MKBSWAP(32)
MKBSWAP(64)

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

/* also contains implementations from in base.h */

void nnc_parse_version(u16 ver, u8 *major, u8 *minor, u8 *patch)
{
	if(major) *major = (ver >> 10) & 0x3F;
	if(minor) *minor = (ver >>  4) & 0x3F;
	if(patch) *patch = (ver      ) & 0xF;
}

// TODO: Test

u16 nnc_tid_category(u64 tid)
{
	return (tid >> 32) & 0xFFFF;
}

u32 nnc_tid_unique_id(u64 tid)
{
	return (tid >> 8) & 0xFFFFFF;
}

u8 nnc_tid_variation(u64 tid)
{
	return (tid) & 0xFF;
}

void nnc_tid_set_category(u64 *tid, u16 category)
{
	((u16 *) (tid))[1] = category;
}

void nnc_tid_set_unique_id(u64 *tid, u32 uniqid)
{
	((u32 *) (tid))[1] = (uniqid << 8) | nnc_tid_variation(*tid);
}

void nnc_tid_set_variation(u64 *tid, u8 variation)
{
	((u8 *) (tid))[7] = variation;
}

void nnc_dumpmem(u8 *mem, u32 len)
{
	for(u32 i = 0; i < len; i += 0x10)
	{
		int row = MIN(0x10, len - i);
		for(int j = 0; j < row; ++j)
			printf("%02X ", mem[i + j]);
		printf("%*s", (0x10 - row) * 3, "");
		printf("| ");
		for(int j = 0; j < row; ++j)
			printf("%c", isgraph(mem[i + j]) ? mem[i + j] : '.');
		puts("");
	}
}

