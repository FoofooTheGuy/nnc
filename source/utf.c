
#include "./internal.h"

static void write_utf8(u8 *out, int outlen, int *outptr, u32 cp)
{
	if(cp < 0x80)
	{
		int n = *outptr + 1;
		if(n <= outlen)
			out[*outptr] = cp;
		*outptr = n;
	}
	else if(cp < 0x800)
	{
		int n = *outptr + 2;
		if(n <= outlen)
		{
			out[*outptr + 0] = (cp >> 6)   | 0xC0;
			out[*outptr + 1] = (cp & 0x3F) | 0x80;
		}
		*outptr = n;
	}
	else if(cp < 0x10000)
	{
		int n = *outptr + 3;
		if(n <= outlen)
		{
			out[*outptr + 0] = (cp >> 12)         | 0xE0;
			out[*outptr + 1] = ((cp >> 6) & 0x3F) | 0x80;
			out[*outptr + 2] = (cp & 0x3F)        | 0x80;
		}
		*outptr = n;
	}
	else if(cp < 0x110000)
	{
		int n = *outptr + 4;
		if(n <= outlen)
		{
			out[*outptr + 0] = (cp >> 18)          | 0xF0;
			out[*outptr + 1] = ((cp >> 12) & 0x3F) | 0x80;
			out[*outptr + 2] = ((cp >> 6) & 0x3F)  | 0x80;
			out[*outptr + 3] = (cp & 0x3F)         | 0x80;
		}
		*outptr = n;
	}
	else { } /* invalid codepoint */
}

/* should there be a BE version of this? */
int nnc_utf16_to_utf8(u8 *out, int outlen, const u16 *in, int inlen)
{
	int outptr = 0;
	for(int i = 0; i < inlen; ++i)
	{
		u16 p1 = LE16(in[i]);
		if(p1 < 0xD800 || p1 > 0xE000)
			write_utf8(out, outlen, &outptr, p1);
		/* surrogate pair */
		else
		{
			u16 p2 = LE16(in[i + 1]);
			u16 w1 = p1 & ~0xD800;
			u16 w2 = p2 & ~0xDC00;
			write_utf8(out, outlen, &outptr, (w1 << 16) | w2);
		}
	}
	return outptr;
}

