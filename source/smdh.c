
#include <nnc/smdh.h>
#include <assert.h>
#include <string.h>
#include "./internal.h"


result nnc_read_smdh(rstream *rs, nnc_smdh *smdh)
{
	assert(sizeof(smdh->titles) == 0x2000 && "smdh->titles was not properly packed");

	u8 header[0x36C0];
	result ret;
	TRY(read_at_exact(rs, 0, header, sizeof(header)));
	/* 0x0000 */ if(memcmp(header, "SMDH", 4) != 0)
	/* 0x0000 */ 	return NNC_R_CORRUPT;
	/* 0x0004 */ smdh->version = LE16P(&header[0x04]);
	/* 0x0006 */ /* reserved */
	/* 0x0008 */ memcpy(smdh->titles, &header[0x0008], sizeof(smdh->titles));
	/* 0x2008 */ memcpy(smdh->game_ratings, &header[0x2008], sizeof(smdh->game_ratings));
	/* 0x2018 */ smdh->lockout = LE32P(&header[0x2018]);
	/* 0x201C */ smdh->match_maker_id = LE32P(&header[0x201C]);
	/* 0x2020 */ smdh->match_maker_bit_id = LE64P(&header[0x2020]);
	/* 0x2028 */ smdh->flags = LE32P(&header[0x2028]);
	/* 0x202C */ smdh->eula_version_minor = header[0x202C];
	/* 0x202C */ smdh->eula_version_major = header[0x202E];
	/* 0x202E */ /* reserved */
	/* 0x2030 */ smdh->optimal_animation_frame = LE32P(&header[0x2030]);
	/* 0x2034 */ smdh->cec_id = LE32P(&header[0x2034]);
	/* 0x2040 */ memcpy(smdh->icon_small, &header[0x2040], 24 * 24 * 2);
	/* 0x24C0 */ memcpy(smdh->icon_large, &header[0x24C0], 48 * 48 * 2);
	return NNC_R_OK;
}

/* N.B.: `from' must be less than `to'! */
#define SHIFT_BIT_TO(var, to, from) (  ((var >> (from)) & 1) << (to)  )
#define UNPACK_U3_SPREAD(var, offset) \
	  SHIFT_BIT_TO(var, 0, 0 + offset) \
	| SHIFT_BIT_TO(var, 1, 2 + offset) \
	| SHIFT_BIT_TO(var, 2, 4 + offset)

#define DEFINE_UNSWIZZLE_FUNC(format_name, inp_type, outp_type, endian_func, vars, conversion_routine)  \
  void nnc_unswizzle_zorder_##format_name(inp_type *inp, outp_type *outp, nnc_u8 dim)                   \
  {                                                                                                     \
    u8 sx, sy, x, y, block;                                                                             \
    inp_type in_colour;                                                                                 \
    outp_type out_colour;                                                                               \
    u32 j = 0;                                                                                          \
    vars                                                                                                \
                                                                                                        \
    /* `dim' must be aligned by 8 due to the blocks being 8x8,                                          \
     * this assert is obviously suboptimal but for the sake                                             \
     * of easier code it's better than returning a result */                                            \
    assert(IS_ALIGNED(dim, 8));                                                                         \
                                                                                                        \
    /* Nintendo swizzles their images in block of 8x8, with those blocks                                \
     *  containing the image data z-order swizzled                                                      \
     * see: https://en.wikipedia.org/wiki/Z-order_curve */                                              \
    for(y = 0; y < dim; y += 8)                                                                         \
      for(x = 0; x < dim; x += 8)                                                                       \
        for(block = 0; block < 8 * 8; ++block)                                                          \
        {                                                                                               \
          /* The range for these is small enough to make a table                                        \
           * but it doesn't pose a large performance problem (afaik)                                    \
           * so i'll just keep it like this for simplicity */                                           \
          sx = UNPACK_U3_SPREAD(block, 0);                                                              \
          sy = UNPACK_U3_SPREAD(block, 1);                                                              \
          in_colour = endian_func(inp[j++]);                                                            \
          conversion_routine                                                                            \
          outp[(y + sy) * dim + x + sx] = out_colour;                                                   \
        }                                                                                               \
  }

#define DEFINE_SWIZZLE_FUNC(format_name, inp_type, outp_type, endian_func, vars, conversion_routine)  \
  void nnc_swizzle_zorder_##format_name(u32 *inp, u16 *outp, u8 dim)                                  \
  {                                                                                                   \
    u8 sx, sy, y, x, block;                                                                           \
    inp_type in_colour;                                                                               \
    outp_type out_colour;                                                                             \
    u32 j = 0;                                                                                        \
    vars                                                                                              \
                                                                                                      \
    assert(IS_ALIGNED(dim, 8));                                                                       \
                                                                                                      \
    for(y = 0; y < dim; y += 8)                                                                       \
      for(x = 0; x < dim; x += 8)                                                                     \
        for(block = 0; block < 8 * 8; ++block)                                                        \
        {                                                                                             \
          sx = UNPACK_U3_SPREAD(block, 0);                                                            \
          sy = UNPACK_U3_SPREAD(block, 1);                                                            \
          in_colour = inp[(y + sy) * dim + x + sx];                                                   \
          conversion_routine                                                                          \
          outp[j++] = endian_func(out_colour);                                                        \
        }                                                                                             \
  }


#define FIX_COLOUR_DEPTH(clr, in_bits, out_bits) \
	( ( ((float) (clr)) / ( (float) ( (1 << in_bits) - 1) ) ) * ( (1 << out_bits) - 1 ) )

DEFINE_UNSWIZZLE_FUNC(le_rgb565_to_rgba8, u16, u32, LE16,
	u8 r;
	u8 g;
	u8 b;
	u8 a = 0xFF; /* since there is no α channel encoded always set it to max */
,
	r = FIX_COLOUR_DEPTH(in_colour >> 0  & 0x1F, 5, 8);
	g = FIX_COLOUR_DEPTH(in_colour >> 5  & 0x3F, 6, 8);
	b = FIX_COLOUR_DEPTH(in_colour >> 11 & 0x1F, 5, 8);
	out_colour = (r << 0) | (g << 8) | (b << 16) | (a << 24);
)

DEFINE_SWIZZLE_FUNC(rgba8_to_le_rgb565, u32, u16, LE16,
	u8 r;
	u8 g;
	u8 b;
,
	r = FIX_COLOUR_DEPTH(in_colour >> 0  & 0xFF, 8, 5);
	g = FIX_COLOUR_DEPTH(in_colour >> 8  & 0xFF, 8, 6);
	b = FIX_COLOUR_DEPTH(in_colour >> 16 & 0xFF, 8, 5);
	out_colour = (r << 0) | (g << 5) | (b << 11);
)

DEFINE_UNSWIZZLE_FUNC(le_rgb565_to_abgr8, u16, u32, LE16,
	u8 r;
	u8 g;
	u8 b;
	u8 a = 0xFF; /* since there is no α channel encoded always set it to max */
,
	r = FIX_COLOUR_DEPTH(in_colour >> 0  & 0x1F, 5, 8);
	g = FIX_COLOUR_DEPTH(in_colour >> 5  & 0x3F, 6, 8);
	b = FIX_COLOUR_DEPTH(in_colour >> 11 & 0x1F, 5, 8);
	out_colour = (a << 0) | (b << 8) | (g << 16) | (r << 24);
)

DEFINE_SWIZZLE_FUNC(abgr8_to_le_rgb565, u32, u16, LE16,
	u8 r;
	u8 g;
	u8 b;
,
	r = FIX_COLOUR_DEPTH(in_colour >> 16 & 0xFF, 8, 5);
	g = FIX_COLOUR_DEPTH(in_colour >> 8  & 0xFF, 8, 6);
	b = FIX_COLOUR_DEPTH(in_colour >> 0  & 0xFF, 8, 5);
	out_colour = (r << 0) | (g << 5) | (b << 11);
)

