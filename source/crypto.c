
#include <mbedtls/sha256.h>
#include <nnc/crypto.h>
#include "./internal.h"

#define MIN(a,b) ((a)<(b)?(a):(b))
#define BLOCK_SZ 0x2000


result nnc_crypto_sha256_view(nnc_rstream *rs, nnc_sha256_hash digest, nnc_u32 size)
{
	mbedtls_sha256_context ctx;
	mbedtls_sha256_init(&ctx);
	mbedtls_sha256_starts_ret(&ctx, 0);
	u8 block[BLOCK_SZ];
	u32 read_left = size, next_read = MIN(size, BLOCK_SZ), read_ret;
	result ret;
	while(read_left != 0)
	{
		ret = NNC_RS_PCALL(rs, read, block, next_read, &read_ret);
		if(ret == NNC_R_OK && read_ret != next_read) ret = NNC_R_TOO_SMALL;
		if(ret != NNC_R_OK)
		{
			mbedtls_sha256_free(&ctx);
			return ret;
		}
		mbedtls_sha256_update_ret(&ctx, block, read_ret);
		read_left -= next_read;
		next_read = MIN(read_left, BLOCK_SZ);
	}
	mbedtls_sha256_finish_ret(&ctx, digest);
	mbedtls_sha256_free(&ctx);
	return NNC_R_OK;
}

