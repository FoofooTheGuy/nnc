// vim: ft=c.doxygen

#ifndef inc_nnc_crypto_h
#define inc_nnc_crypto_h

#include <nnc/read-stream.h>
#include <nnc/base.h>
NNC_START

/** Buffer large enough to hold a sha256 hash. */
typedef nnc_u8 nnc_sha256_hash[0x20];

/** \brief          Hash a \ref rstream partly.
 *  \param rs       Stream to hash.
 *  \param digest   Output digest.
 *  \param size     Amount of data to hash.
 */
nnc_result nnc_crypto_sha256_view(nnc_rstream *rs, nnc_sha256_hash digest, nnc_u32 size);

NNC_END
#endif

