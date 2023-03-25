
#include <nnc/exefs.h>
#include <nnc/romfs.h>
#include <nnc/ncch.h>
#include <string.h>
#include <stdlib.h>
#include "./internal.h"

#define EXHEADER_OFFSET NNC_MEDIA_UNIT
#define EXHEADER_FULL_SIZE (2*EXHEADER_NCCH_SIZE)
#define EXHEADER_NCCH_SIZE 0x400


result nnc_read_ncch_header(rstream *rs, nnc_ncch_header *ncch)
{
	u8 header[0x200];
	result ret;

	TRY(read_at_exact(rs, 0, header, sizeof(header)));
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
	/* 0x114 */ memcpy(ncch->seed_hash, &header[0x114], 4);
	/* 0x118 */ ncch->title_id = LE64P(&header[0x118]);
	/* 0x120 */ /* reserved */
	/* 0x130 */ memcpy(ncch->logo_hash, &header[0x130], sizeof(nnc_sha256_hash));
	/* 0x150 */ memcpy(ncch->product_code, &header[0x150], 0x10);
	/* 0x150 */ ncch->product_code[0x10] = '\0';
	/* 0x160 */ memcpy(ncch->exheader_hash, &header[0x160], sizeof(nnc_sha256_hash));
	/* 0x180 */ ncch->exheader_size = LE32P(&header[0x180]);
	/* 0x184 */ /* reserved */
	/* 0x188 */ /* ncchflags[0] */
	/* 0x189 */ /* ncchflags[1] */
	/* 0x18A */ /* ncchflags[2] */
	/* 0x18B */ ncch->crypt_method = header[0x18B];
	/* 0x18C */ ncch->platform = header[0x18C];
	/* 0x18D */ ncch->type = header[0x18D];
	/* 0x18E */ ncch->content_unit = NNC_MEDIA_UNIT * nnc_pow2(header[0x18E]);
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

#define SUBVIEW_R(mode, offset, size) \
	nnc_subview_open(&section->u. mode .sv, rs, offset, size)
#define SUBVIEW(mode, offset, size) \
	SUBVIEW_R(mode, NNC_MU_TO_BYTE(offset), NNC_MU_TO_BYTE(size))

result nnc_ncch_section_romfs(nnc_ncch_header *ncch, nnc_rstream *rs,
	nnc_keypair *kp, nnc_ncch_section_stream *section)
{
	if(ncch->flags & NNC_NCCH_NO_ROMFS || ncch->romfs_size == 0)
		return NNC_R_NOT_FOUND;
	if(ncch->flags & NNC_NCCH_NO_CRYPTO)
		return SUBVIEW(dec, ncch->romfs_offset, ncch->romfs_size), NNC_R_OK;

	u8 iv[0x10]; result ret;
	TRY(nnc_get_ncch_iv(ncch, NNC_SECTION_ROMFS, iv));
	SUBVIEW(enc, ncch->romfs_offset, ncch->romfs_size);
	return nnc_aes_ctr_open(&section->u.enc.crypt, NNC_RSP(&section->u.enc.sv),
		&kp->secondary, iv);
}

result nnc_ncch_section_exefs_header(nnc_ncch_header *ncch, nnc_rstream *rs,
	nnc_keypair *kp, nnc_ncch_section_stream *section)
{
	if(ncch->exefs_size == 0) return NNC_R_NOT_FOUND;
	/* ExeFS header is only 1 NNC_MEDIA_UNIT large */
	if(ncch->flags & NNC_NCCH_NO_CRYPTO)
		return SUBVIEW(dec, ncch->exefs_offset, 1), NNC_R_OK;

	u8 iv[0x10]; result ret;
	TRY(nnc_get_ncch_iv(ncch, NNC_SECTION_EXEFS, iv));
	SUBVIEW(enc, ncch->exefs_offset, 1);
	return nnc_aes_ctr_open(&section->u.enc.crypt, NNC_RSP(&section->u.enc.sv),
		&kp->primary, iv);
}

nnc_result nnc_ncch_section_exheader(nnc_ncch_header *ncch, nnc_rstream *rs,
	nnc_keypair *kp, nnc_ncch_section_stream *section)
{
	if(ncch->exheader_size == 0) return NNC_R_NOT_FOUND;
	/* for some reason the header says it's 0x400 bytes whilest it really is 0x800 bytes */
	if(ncch->exheader_size != EXHEADER_NCCH_SIZE) return NNC_R_CORRUPT;
	if(ncch->flags & NNC_NCCH_NO_CRYPTO)
		return SUBVIEW_R(dec, EXHEADER_OFFSET, EXHEADER_NCCH_SIZE), NNC_R_OK;

	u8 iv[0x10]; result ret;
	TRY(nnc_get_ncch_iv(ncch, NNC_SECTION_EXHEADER, iv));
	SUBVIEW_R(enc, EXHEADER_OFFSET, EXHEADER_FULL_SIZE);
	return nnc_aes_ctr_open(&section->u.enc.crypt, NNC_RSP(&section->u.enc.sv),
		&kp->primary, iv);
}

nnc_result nnc_ncch_exefs_subview(nnc_ncch_header *ncch, nnc_rstream *rs,
	nnc_keypair *kp, nnc_ncch_section_stream *section, nnc_exefs_file_header *header)
{
	if(ncch->exefs_size == 0) return NNC_R_NOT_FOUND;
	if(ncch->flags & NNC_NCCH_NO_CRYPTO)
		return SUBVIEW_R(dec,
			NNC_MU_TO_BYTE(ncch->exefs_offset) + NNC_EXEFS_HEADER_SIZE + header->offset,
			header->size), NNC_R_OK;

	nnc_u128 *key; u8 iv[0x10]; result ret;
	TRY(nnc_get_ncch_iv(ncch, NNC_SECTION_EXEFS, iv));
	nnc_u128 ctr = nnc_u128_import_be(iv);
	/* we need to advance the IV to the correct section, split the header so the
	 * compiler hopefully has an easier job optimizing that */
	nnc_u128 addition = NNC_PROMOTE128((header->offset / 0x10) + (NNC_EXEFS_HEADER_SIZE / 0x10));
	nnc_u128_add(&ctr, &addition);
	nnc_u128_bytes_be(&ctr, iv);

	/* these belong to the "info menu" group */
	if(strcmp(header->name, "icon") == 0 || strcmp(header->name, "banner") == 0)
		key = &kp->primary;
	/* the rest belongs to the "content" group */
	else
		key = &kp->secondary;

	SUBVIEW_R(enc,
		NNC_MU_TO_BYTE(ncch->exefs_offset) + NNC_EXEFS_HEADER_SIZE + header->offset,
		header->size);
	return nnc_aes_ctr_open(&section->u.enc.crypt, NNC_RSP(&section->u.enc.sv), key, iv);
}

nnc_result nnc_ncch_section_plain(nnc_ncch_header *ncch, nnc_rstream *rs,
	nnc_subview *section)
{
	if(ncch->plain_size == 0) return NNC_R_NOT_FOUND;
	nnc_subview_open(section, rs, NNC_MU_TO_BYTE(ncch->plain_offset),
		NNC_MU_TO_BYTE(ncch->plain_size));
	return 0;
}

nnc_result nnc_ncch_section_logo(nnc_ncch_header *ncch, nnc_rstream *rs,
	nnc_subview *section)
{
	if(ncch->logo_size == 0) return NNC_R_NOT_FOUND;
	nnc_subview_open(section, rs, NNC_MU_TO_BYTE(ncch->logo_offset),
		NNC_MU_TO_BYTE(ncch->logo_size));
	return 0;
}

nnc_result nnc_write_ncch(
	nnc_ncch_header *ncch_header,
	nnc_u8 wflags,
	nnc_exheader_or_stream exheader,
	nnc_rstream *logo,
	nnc_rstream *plain,
	nnc_vfs_or_stream_or_voidp exefs,
	nnc_vfs_or_stream_or_voidp romfs,
	nnc_wstream *ws)
{
	result ret;
	u32 header_off, end_off, logo_off = 0, plain_off = 0, exefs_off = 0, romfs_off = 0, logo_size = 0, plain_size = 0, exefs_size = 0, romfs_size = 0;
	nnc_sha256_hash exheader_hash, logo_hash, exefs_super_hash, romfs_super_hash;
	nnc_hasher_writer hwrite;
	nnc_header_saver hsaver;
	u8 header[0x200];

	if(!ws->funcs->seek)
		return NNC_R_INVAL;

	memset(&exheader_hash, 0x00, sizeof(exheader_hash));
	memset(&logo_hash, 0x00, sizeof(logo_hash));
	memset(&exefs_super_hash, 0x00, sizeof(exefs_super_hash));
	memset(&romfs_super_hash, 0x00, sizeof(romfs_super_hash));

	/* we'll reserve space for the header as we'll write it last */
	header_off = NNC_WS_PCALL0(ws, tell);
	/* the exheader resides directly after the header */
	TRY(nnc_write_padding(ws, EXHEADER_OFFSET));

	if(wflags & NNC_NCCH_WF_EXHEADER_WRITE)
		return NNC_R_UNSUPPORTED; /* unsupported for now */
	else
	{
		if(NNC_RS_PCALL0(exheader.stream, size) != EXHEADER_FULL_SIZE)
			return NNC_R_INVAL;
		TRY(nnc_open_hasher_writer(&hwrite, ws, EXHEADER_NCCH_SIZE));
		ret = nnc_copy(exheader.stream, NNC_WSP(&hwrite), NULL);
		nnc_hasher_writer_digest(&hwrite, exheader_hash);
		if(ret != NNC_R_OK) return ret;
	}

	if(logo)
	{
		logo_off = NNC_WS_PCALL0(ws, tell);
		TRY(nnc_open_hasher_writer(&hwrite, ws, 0));
		ret = nnc_copy(logo, NNC_WSP(&hwrite), &logo_size);
		if(ret == NNC_R_OK)
			ret = nnc_write_padding(NNC_WSP(&hwrite), ALIGN(logo_size, NNC_MEDIA_UNIT) - logo_size);
		nnc_hasher_writer_digest(&hwrite, logo_hash);
		if(ret != NNC_R_OK) return ret;
		if(logo_size == 0) logo_off = 0;
	}

	if(plain)
	{
		plain_off = NNC_WS_PCALL0(ws, tell);
		TRY(nnc_copy(plain, ws, &plain_size));
		TRY(nnc_write_padding(ws, ALIGN(plain_size, NNC_MEDIA_UNIT) - plain_size));
		if(plain_size == 0) plain_off = 0;
	}

	if(exefs.voidp)
	{
		exefs_off = NNC_WS_PCALL0(ws, tell);
		if(wflags & NNC_NCCH_WF_EXEFS_VFS)
		{
			TRY(nnc_open_header_saver(&hsaver, ws, NNC_MEDIA_UNIT));
			ret = nnc_write_exefs(exefs.vfs, NNC_WSP(&hsaver));
			exefs_size = NNC_WS_PCALL0(ws, tell) - exefs_off;
			if(exefs_size >= NNC_MEDIA_UNIT)
				nnc_crypto_sha256_buffer(hsaver.buffer, NNC_MEDIA_UNIT, exefs_super_hash);
			NNC_WS_CALL0(hsaver, close);
			if(ret == NNC_R_OK && exefs_size < NNC_MEDIA_UNIT)
				ret = NNC_R_INVAL; /* shouldn't happen afaik */
			if(ret != NNC_R_OK) return ret;
		}
		else
		{
			if((exefs_size = NNC_RS_PCALL0(exefs.stream, size)) < NNC_MEDIA_UNIT)
				return NNC_R_INVAL; /* a valid ExeFS has at least NNC_MEDIA_UNIT bytes */
			TRY(nnc_open_hasher_writer(&hwrite, ws, NNC_MEDIA_UNIT));
			ret = nnc_copy(exefs.stream, NNC_WSP(&hwrite), NULL);
			nnc_hasher_writer_digest(&hwrite, exefs_super_hash);
			if(ret != NNC_R_OK) return ret;
		}
		TRY(nnc_write_padding(ws, ALIGN(exefs_size, NNC_MEDIA_UNIT) - exefs_size));
	}

	if(romfs.voidp)
	{
		romfs_off = NNC_WS_PCALL0(ws, tell);
		if(wflags & NNC_NCCH_WF_ROMFS_VFS)
		{
			TRY(nnc_open_header_saver(&hsaver, ws, NNC_MEDIA_UNIT));
			ret = nnc_write_romfs(romfs.vfs, NNC_WSP(&hsaver));
			romfs_size = NNC_WS_PCALL0(ws, tell) - romfs_off;
			if(romfs_size >= NNC_MEDIA_UNIT)
				nnc_crypto_sha256_buffer(hsaver.buffer, NNC_MEDIA_UNIT, romfs_super_hash);
			NNC_WS_CALL0(hsaver, close);
			if(ret == NNC_R_OK && romfs_size < NNC_MEDIA_UNIT)
				ret = NNC_R_INVAL; /* shouldn't happen afaik */
			if(ret != NNC_R_OK) return ret;
		}
		else
		{
			if((romfs_size = NNC_RS_PCALL0(romfs.stream, size)) < NNC_MEDIA_UNIT)
				return NNC_R_INVAL; /* a valid RomFS has at least NNC_MEDIA_UNIT bytes */
			TRY(nnc_open_hasher_writer(&hwrite, ws, NNC_MEDIA_UNIT));
			ret = nnc_copy(romfs.stream, NNC_WSP(&hwrite), NULL);
			nnc_hasher_writer_digest(&hwrite, romfs_super_hash);
			if(ret != NNC_R_OK) return ret;
		}
		TRY(nnc_write_padding(ws, ALIGN(romfs_size, NNC_MEDIA_UNIT) - romfs_size));
		if(romfs_size == 0) romfs_off = 0;
	}

	/* now comes the header */
	end_off = NNC_WS_PCALL0(ws, tell);
	TRY(NNC_WS_PCALL(ws, seek, header_off));

	/* convert everything to media units... */
	logo_off   = NNC_BYTE_TO_MU(logo_off);
	plain_off  = NNC_BYTE_TO_MU(plain_off);
	exefs_off  = NNC_BYTE_TO_MU(exefs_off);
	romfs_off  = NNC_BYTE_TO_MU(romfs_off);
	logo_size  = NNC_BYTE_TO_MU(logo_size);
	plain_size = NNC_BYTE_TO_MU(plain_size);
	exefs_size = NNC_BYTE_TO_MU(exefs_size);
	romfs_size = NNC_BYTE_TO_MU(romfs_size);

	char product_code[0x10 + 1];
	memset(product_code, 0x00, sizeof(product_code));
	strcpy(product_code, ncch_header->product_code);

	/* 0x000 */ memset(&header[0x000], 0x00, 0x100);
	/* 0x100 */ memcpy(&header[0x100], "NCCH", 4);
	/* 0x104 */ U32P(&header[0x104]) = LE32(romfs_off + romfs_size); /* content size */
	/* 0x108 */ U64P(&header[0x108]) = LE64(ncch_header->partition_id);
	/* 0x110 */ memcpy(&header[0x110], ncch_header->maker_code, 2);
	/* 0x112 */ U16P(&header[0x112]) = LE16(2);
	/* 0x114 */ memset(&header[0x114], 0x00, 4); /* seed hash, crypto not supported yet */
	/* 0x118 */ U64P(&header[0x118]) = LE64(ncch_header->title_id);
	/* 0x120 */ memset(&header[0x120], 0x00, 0x10); /* reserved */
	/* 0x130 */ memcpy(&header[0x130], logo_hash, 0x20); /* logo region hash */
	/* 0x150 */ memcpy(&header[0x150], product_code, 0x10);
	/* 0x160 */ memcpy(&header[0x160], exheader_hash, 0x20); /* exheader region hash (initial 0x400) */
	/* 0x180 */ U32P(&header[0x180]) = LE32(EXHEADER_NCCH_SIZE); /* "exheader size," but better named "exheader hash size" */
	/* 0x184 */ U32P(&header[0x184]) = 0; /* reserved */
	/* 0x188 */ header[0x188] = 0; /* ncchflags[0] */
	/* 0x189 */ header[0x189] = 0; /* ncchflags[1] */
	/* 0x18A */ header[0x18A] = 0; /* ncchflags[2] */
	/* 0x18B */ header[0x18B] = NNC_CRYPT_INITIAL; /* crypto method, unsupported for now */
	/* 0x18C */ header[0x18C] = ncch_header->platform;
	/* 0x18D */ header[0x18D] = ncch_header->type;
	/* 0x18E */ header[0x18E] = 0; /* content unit size; 0x200*2^0=0x200 (=NNC_MEDIA_UNIT) */
	/* 0x18F */ header[0x18F] = NNC_NCCH_NO_CRYPTO; /* flags */
	/* 0x18F */ if(!romfs_size) header[0x18F] |= NNC_NCCH_NO_ROMFS;
	/* 0x190 */ U32P(&header[0x190]) = LE32(plain_off);
	/* 0x194 */ U32P(&header[0x194]) = LE32(plain_size);
	/* 0x198 */ U32P(&header[0x198]) = LE32(logo_off);
	/* 0x19C */ U32P(&header[0x19C]) = LE32(logo_size);
	/* 0x1A0 */ U32P(&header[0x1A0]) = LE32(exefs_off);
	/* 0x1A4 */ U32P(&header[0x1A4]) = LE32(exefs_size);
	/* 0x1A8 */ U32P(&header[0x1A8]) = LE32(1); /* ExeFS hash region size (1 MU (0x200) suffices for the hashed header) */
	/* 0x1AC */ U32P(&header[0x1AC]) = 0; /* reserved */
	/* 0x1B0 */ U32P(&header[0x1B0]) = LE32(romfs_off);
	/* 0x1B4 */ U32P(&header[0x1B4]) = LE32(romfs_size);
	/* 0x1B8 */ U32P(&header[0x1B8]) = LE32(1); /* RomFS hash region size (1 MU (0x200) suffices for the IVFC master hash) */
	/* 0x1BC */ U32P(&header[0x1BC]) = 0; /* reserved */
	/* 0x1C0 */ memcpy(&header[0x1C0], exefs_super_hash, 0x20); /* ExeFS superblock hash */
	/* 0x1E0 */ memcpy(&header[0x1E0], romfs_super_hash, 0x20); /* RomFS superblock hash */

	TRY(NNC_WS_PCALL(ws, write, header, sizeof(header)));
	TRY(NNC_WS_PCALL(ws, seek, end_off));

	return NNC_R_OK;
}

