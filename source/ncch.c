
#include <mbedtls/aes.h>
#include <nnc/ncch.h>
#include <string.h>
#include <stdlib.h>
#include "./internal.h"

#define EXHEADER_OFFSET NNC_MEDIA_UNIT


static u32 u32pow(u32 x, u8 y)
{
	u32 ret = 1;
	for(u32 i = 0; i < y; ++i)
		ret *= x;
	return ret;
}

result nnc_read_ncch_header(rstream *rs, nnc_ncch_header *ncch)
{
	u8 header[0x200];
	result ret;

	TRY(read_exact(rs, header, sizeof(header)));
	/* 0x000 */ ncch->keyy = nnc_u128_import_be(&header[0x000]);
	/* 0x000 */ /* signature (and keyY) */
	/* 0x100 */ if(memcmp(&header[0x100], "NCCH", 4) != 0)
	/* 0x100 */ 	return NNC_R_CORRUPT;
	/* 0x104 */ ncch->content_size = LE32P(&header[0x104]);
	/* 0x108 */ ncch->partition_id = LE64P(&header[0x108]);
	/* 0x110 */ ncch->maker_code[0] = header[0x110];
	/* 0x111 */ ncch->maker_code[1] = header[0x111];
	/* 0x111 */ ncch->maker_code[2] = '\0';
	/* 0x112 */ ncch->version = LE16P(&header[0x112]);
	/* 0x114 */ ncch->seed_hash = U32P(&header[0x114]);
	/* 0x118 */ ncch->title_id = LE64P(&header[0x118]);
	/* 0x120 */ /* reserved */
	/* 0x130 */ memcpy(ncch->logo_hash, &header[0x130], sizeof(nnc_sha256_hash));
	/* 0x150 */ memcpy(ncch->product_code, &header[0x150], 0x10);
	/* 0x150 */ ncch->product_code[0x10] = '\0';
	/* 0x160 */ memcpy(ncch->extheader_hash, &header[0x160], sizeof(nnc_sha256_hash));
	/* 0x180 */ ncch->extheader_size = LE32P(&header[0x180]);
	/* 0x184 */ /* reserved */
	/* 0x188 */ /* ncchflags[0] */
	/* 0x189 */ /* ncchflags[1] */
	/* 0x18A */ /* ncchflags[2] */
	/* 0x18B */ ncch->crypt_method = header[0x18B];
	/* 0x18C */ ncch->platform = header[0x18C];
	/* 0x18D */ ncch->type = header[0x18D];
	/* 0x18E */ ncch->content_unit = NNC_MEDIA_UNIT * u32pow(2, header[0x18E]);
	/* 0x18F */ ncch->flags = header[0x18F];
	/* 0x190 */ ncch->plain_offset = LE32P(&header[0x190]);
	/* 0x194 */ ncch->plain_size = LE32P(&header[0x194]);
	/* 0x198 */ ncch->logo_offset = LE32P(&header[0x198]);
	/* 0x19C */ ncch->logo_size = LE32P(&header[0x19C]);
	/* 0x1A0 */ ncch->exefs_offset = LE32P(&header[0x1A0]);
	/* 0x1A4 */ ncch->exefs_size = LE32P(&header[0x1A4]);
	/* 0x1A8 */ ncch->exefs_hash_size = LE32P(&header[0x1A8]);
	/* 0x1AC */ /* reserved */
	/* 0x1B0 */ ncch->romfs_offset = LE32P(&header[0x1B0]);
	/* 0x1B4 */ ncch->romfs_size = LE32P(&header[0x1B4]);
	/* 0x1B8 */ ncch->romfs_hash_size = LE32P(&header[0x1B8]);
	/* 0x1BC */ /* reserved */
	/* 0x1C0 */ memcpy(ncch->exefs_hash, &header[0x1C0], sizeof(nnc_sha256_hash));
	/* 0x1E0 */ memcpy(ncch->romfs_hash, &header[0x1E0], sizeof(nnc_sha256_hash));
	return NNC_R_OK;
}

static result open_sv(nnc_subview *section, rstream *rs, nnc_ncch_header *ncch,
	u8 which)
{
	switch(which)
	{
	case NNC_SECTION_ROMFS:
		nnc_subview_open(section, rs, NNC_MU_TO_BYTE(ncch->romfs_offset),
			NNC_MU_TO_BYTE(ncch->romfs_size));
		break;
	case NNC_SECTION_EXEFS:
		// TODO...
	case NNC_SECTION_EXTHEADER:
		break;
	default:
		fprintf(stderr, "Invalid section given in open_sv()\n");
		abort();
	}
	return NNC_R_OK;
}

static result open_content_section(nnc_ncch_header *ncch, nnc_rstream *rs,
	nnc_seeddb *seeddb, nnc_keyset *ks, nnc_ncch_section_stream *section, u8 which)
{
	if(ncch->flags & NNC_NCCH_NO_CRYPTO)
		return open_sv(&section->u.dec.sv, rs, ncch, which);

	result ret; u8 iv[0x10]; u128 key;
	TRY(nnc_get_ncch_iv(ncch, which, iv));
	TRY(nnc_key_content(&key, ks, ncch, seeddb));

	TRY(open_sv(&section->u.enc.sv, rs, ncch, which));
	TRY(nnc_aes_ctr_open(&section->u.enc.crypt, NNC_RSP(&section->u.enc.sv), &key, iv));
	return NNC_R_OK;
}

result nnc_ncch_section_romfs(nnc_ncch_header *ncch, nnc_rstream *rs,
	nnc_seeddb *seeddb, nnc_keyset *ks, nnc_ncch_section_stream *romfs_section)
{
	if(ncch->flags & NNC_NCCH_NO_ROMFS) return NNC_R_NOT_FOUND;
	return open_content_section(ncch, rs, seeddb, ks, romfs_section, NNC_SECTION_ROMFS);
}

