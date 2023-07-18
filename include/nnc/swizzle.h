/** \file  swizzle.h
 *  \brief Functions relating to image swizzling.
 */
#ifndef inc_nnc_swizzle_h
#define inc_nnc_swizzle_h

#include <nnc/base.h>
NNC_BEGIN

/** \brief       Unswizzles a z-order curve rgb565 (little endian) image to an rgba8 (native endian) image.
 *  \param inp   Input array, should be at least of size `dim*dim*sizeof(nnc_u16)`.
 *  \param outp  Output array, should be at least of size `dim*dim*sizeof(nnc_u32)`.
 *  \param x     Image width, must be aligned by 8.
 *  \param y     Image height, must be aligned by 8.
 */
void nnc_unswizzle_zorder_le_rgb565_to_rgba8(nnc_u16 *inp, nnc_u32 *outp, nnc_u16 x, nnc_u16 y);

/** \brief       Unswizzles a z-order curve rgb565 (little endian) image to an abgr8 (native endian) image.
 *  \param inp   Input array, should be at least of size `dim*dim*sizeof(nnc_u16)`.
 *  \param outp  Output array, should be at least of size `dim*dim*sizeof(nnc_u32)`.
 *  \param x     Image width, must be aligned by 8.
 *  \param y     Image height, must be aligned by 8.
 */
void nnc_unswizzle_zorder_le_rgb565_to_abgr8(nnc_u16 *inp, nnc_u32 *outp, nnc_u16 x, nnc_u16 y);

/** \brief       Swizzles an rgba8 (native endian) image to a z-order curve rgb565 (little endian) image.
 *  \param inp   Input array, should be at least of size `dim*dim*sizeof(nnc_u32)`.
 *  \param outp  Output array, should be at least of size `dim*dim*sizeof(nnc_u16)`.
 *  \param x     Image width, must be aligned by 8.
 *  \param y     Image height, must be aligned by 8.
 */
void nnc_swizzle_zorder_rgba8_to_le_rgb565(nnc_u32 *inp, nnc_u16 *outp, nnc_u16 x, nnc_u16 y);

/** \brief       Swizzles an abgr8 (native endian) image to a z-order curve rgb565 (little endian) image.
 *  \param inp   Input array, should be at least of size `dim*dim*sizeof(nnc_u32)`.
 *  \param outp  Output array, should be at least of size `dim*dim*sizeof(nnc_u16)`.
 *  \param x     Image width, must be aligned by 8.
 *  \param y     Image height, must be aligned by 8.
 */
void nnc_swizzle_zorder_abgr8_to_le_rgb565(nnc_u32 *inp, nnc_u16 *outp, nnc_u16 x, nnc_u16 y);

NNC_END
#endif

