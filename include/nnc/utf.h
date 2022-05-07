// vim: ft=c.doxygen
#ifndef inc_nnc_utf_h
#define inc_nnc_utf_h
/** \file  utf.h
 *  \brief UTF conversion.
 */
#include <nnc/base.h>
NNC_START

/** \brief         Converts a UTF16 string to a UTF8 one.
 *  \note          The NULL terminator is never placed.
 *  \param out     Output UTF8 string. Should be of size (\p inlen * 2) to always fit the entire output.
 *  \param outlen  Length of \p out.
 *  \param in      Input UTF16 string.
 *  \param inlen   Length of \p in.
 *  \returns       Total size of the converted string, even if buffer was too small.
 */
int nnc_utf16_to_utf8(nnc_u8 *out, int outlen, const nnc_u16 *in, int inlen);

NNC_END
#endif

