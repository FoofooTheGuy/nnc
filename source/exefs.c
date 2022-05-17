
#include <nnc/exefs.h>
#include <string.h>
#include <stdlib.h>
#include "./internal.h"


bool nnc_exefs_file_in_use(nnc_exefs_file_header *fh)
{
	return fh->name[0] != '\0';
}

result nnc_read_exefs_header(nnc_rstream *rs, nnc_exefs_file_header *headers,
	nnc_sha256_hash *hashes, nnc_u8 *size)
{
	result ret;
	u8 i = 0;
	if(headers)
	{
		u8 data[0xA0];
		u8 *cur = data;
		TRY(read_at_exact(rs, 0x0, data, sizeof(data)));
		for(; i < NNC_EXEFS_MAX_FILES && cur[0] != '\0'; ++i, cur = &data[0x10 * i])
		{
			/* 0x00 */ memcpy(headers[i].name, cur, 8);
			/* 0x00 */ headers[i].name[8] = '\0';
			/* 0x08 */ headers[i].offset = LE32P(&cur[0x8]);
			/* 0x0C */ headers[i].size = LE32P(&cur[0xC]);
		}
		headers[i].name[0] = '\0';
	}
	if(hashes)
	{
		nnc_sha256_hash data[0x140];
		TRY(read_at_exact(rs, 0xC0, (u8 *) data, sizeof(data)));
		/* we can trust i */
		if(headers)
			for(u8 j = 0; j < i; ++j)
				memcpy(hashes[j], data[NNC_EXEFS_MAX_FILES - j - 1], sizeof(*hashes));
		else
			for(; i < NNC_EXEFS_MAX_FILES; ++i)
				memcpy(hashes[i], data[NNC_EXEFS_MAX_FILES - i - 1], sizeof(*hashes));
	}
	if(size) *size = i;

	return NNC_R_OK;
}

i8 nnc_find_exefs_file_index(const char *name, nnc_exefs_file_header *headers)
{
	u32 len = strlen(name);
	if(len > 8) return -1;
	++len;
	for(u8 i = 0; i < NNC_EXEFS_MAX_FILES && nnc_exefs_file_in_use(&headers[i]); ++i)
	{
		if(memcmp(name, headers[i].name, len) == 0)
			return i;
	}
	return -1;
}

void nnc_exefs_subview(nnc_rstream *rs, nnc_subview *sv, nnc_exefs_file_header *header)
{
	nnc_subview_open(sv, rs, NNC_EXEFS_HEADER_SIZE + header->offset, header->size);
}

bool nnc_verify_file(nnc_rstream *rs, nnc_exefs_file_header *headers,
	nnc_sha256_hash *hashes, nnc_u8 i)
{
	nnc_sha256_hash hash;
	NNC_RS_PCALL(rs, seek_abs, NNC_EXEFS_HEADER_SIZE + headers[i].offset);
	TRYB(nnc_crypto_sha256_part(rs, hash, headers[i].size));
	return memcmp(hash, hashes[i], sizeof(hash)) == 0;
}

