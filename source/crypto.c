
#include <mbedtls/version.h>
#include <mbedtls/sha256.h>
#include <mbedtls/aes.h>
#include <nnc/crypto.h>
#include <nnc/ncch.h>
#include <stdlib.h>
#include <string.h>
#include "./internal.h"

#define BLOCK_SZ 0x10000

/* In MbedTLS version 2 the normal functions were marked deprecated
 * you were supposed to use *_ret, but in mbedTLS version 3+ the
 * *_ret functions had the functions renamed to have the _ret suffix removed */
#if MBEDTLS_VERSION_MAJOR == 2
	#define mbedtls_sha256_starts mbedtls_sha256_starts_ret
	#define mbedtls_sha256_update mbedtls_sha256_update_ret
	#define mbedtls_sha256_finish mbedtls_sha256_finish_ret
#endif


result nnc_crypto_sha256_part(nnc_rstream *rs, nnc_sha256_hash digest, u32 size)
{
	mbedtls_sha256_context ctx;
	mbedtls_sha256_init(&ctx);
	mbedtls_sha256_starts(&ctx, 0);
	u8 block[BLOCK_SZ];
	u32 read_left = size, next_read = MIN(size, BLOCK_SZ), read_ret;
	result ret;
	while(read_left != 0)
	{
		ret = NNC_RS_PCALL(rs, read, block, next_read, &read_ret);
		if(ret != NNC_R_OK) goto out;
		if(read_ret != next_read) { ret = NNC_R_TOO_SMALL; goto out; }
		mbedtls_sha256_update(&ctx, block, read_ret);
		read_left -= next_read;
		next_read = MIN(read_left, BLOCK_SZ);
	}
	mbedtls_sha256_finish(&ctx, digest);
	ret = NNC_R_OK;
out:
	mbedtls_sha256_free(&ctx);
	return ret;
}

result nnc_crypto_sha256(const u8 *buf, nnc_sha256_hash digest, u32 size)
{
	mbedtls_sha256_context ctx;
	mbedtls_sha256_init(&ctx);
	mbedtls_sha256_starts(&ctx, 0);
	mbedtls_sha256_update(&ctx, buf, size);
	mbedtls_sha256_finish(&ctx, digest);
	mbedtls_sha256_free(&ctx);
	return NNC_R_OK;
}

#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
	#include <unistd.h>
	#define UNIX_LIKE
	#define can_read(f) access(f, R_OK) == 0
#elif defined(_WIN32)
	#include <io.h>
	#define can_read(f) _access_s(f, 4) == 0
#endif

static bool find_support_file(const char *name, char *output)
{
	char pathbuf[1024 + 1];
#define CHECK_BASE(...) do { \
		snprintf(pathbuf, 1024, __VA_ARGS__); \
		if(can_read(pathbuf)) \
		{ \
			strcpy(output, pathbuf); \
			return true; \
		}\
	} while(0)
	char *envptr;
#define CHECKE(path) CHECK_BASE("%s/%s/%s", envptr, path, name)
#define CHECK(path) CHECK_BASE("%s/%s", path, name)

#ifdef UNIX_LIKE
	if((envptr = getenv("HOME")))
	{
		CHECKE(".config/3ds");
		CHECKE("3ds");
		CHECKE(".3ds");
	}
	CHECK("/usr/share/3ds");
#elif defined(_WIN32)
	if((envptr = getenv("USERPROFILE")))
	{
		CHECKE("3ds");
		CHECKE(".3ds");
	}
	if((envptr = getenv("APPDATA")))
		CHECKE("3ds");
#else
	/* no clue where to look */
	(void) pathbuf;
	(void) envptr;
#endif

	/* nothing found */
	return false;
#undef CHECK_BASE
#undef CHECKE
#undef CHECK
}

nnc_result nnc_seeds_seeddb(nnc_rstream *rs, nnc_seeddb *seeddb)
{
	u8 buf[0x20];
	result ret;
	seeddb->size = 0;
	u32 expected_size;
	TRY(read_exact(rs, buf, 0x10));
	expected_size = LE32P(&buf[0x00]);
	if(!(seeddb->entries = malloc(expected_size * sizeof(struct nnc_seeddb_entry))))
		return NNC_R_NOMEM;
	for(u32 i = 0; i < expected_size; ++i, ++seeddb->size)
	{
		TRY(read_exact(rs, buf, 0x20));
		memcpy(seeddb->entries[i].seed, &buf[0x08], NNC_SEED_SIZE);
		seeddb->entries[i].title_id = LE64P(&buf[0x00]);
	}
	return NNC_R_OK;
}

result nnc_scan_boot9(nnc_keyset *ks)
{
	char path[1024 + 1];
	if(!find_support_file("boot9.bin", path))
		return NNC_R_NOT_FOUND;
	nnc_file f;
	result ret;
	TRY(nnc_file_open(&f, path));
	ret = nnc_keyset_boot9(NNC_RSP(&f), ks, 0);
	NNC_RS_CALL0(f, close);
	return ret;
}

result nnc_scan_seeddb(nnc_seeddb *seeddb)
{
	char path[1024 + 1];
	if(!find_support_file("seeddb.bin", path))
		return NNC_R_NOT_FOUND;
	nnc_file f;
	result ret;
	TRY(nnc_file_open(&f, path));
	ret = nnc_seeds_seeddb(NNC_RSP(&f), seeddb);
	NNC_RS_CALL0(f, close);
	return ret;
}

u8 *nnc_get_seed(nnc_seeddb *seeddb, u64 tid)
{
	for(u32 i = 0; i < seeddb->size; ++i)
	{
		if(seeddb->entries[i].title_id == tid)
			return seeddb->entries[i].seed;
	}
	return NULL;
}

void nnc_free_seeddb(nnc_seeddb *seeddb)
{
	free(seeddb->entries);
}

enum keyfield {
	BOOTROM = 0x01,
	DEFAULT = 0x02,
	DEV     = 0x04,
	RETAIL  = 0x08,

	TYPE_FIELD = DEV | RETAIL,
};

#define TYPE_FLAG(is_dev) (is_dev ? DEV : RETAIL)

result nnc_keyset_boot9(nnc_rstream *rs, nnc_keyset *ks, bool dev)
{
	u8 type = ks->flags & TYPE_FIELD, mtype = TYPE_FLAG(dev);
	if(type && type != TYPE_FLAG(dev))
		return NNC_R_MISMATCH;
	result ret;
	u8 blob[0x10];
	TRY(read_at_exact(rs, dev ? 0x5DD0 : 0x59D0, blob, sizeof(blob)));
	/* there are more keys in the bootrom but for now they're
	 * not needed in nnc */
	ks->kx_ncch0 = nnc_u128_import_be(&blob[0x00]);
	ks->flags |= BOOTROM | mtype;
	return NNC_R_OK;
}

static const struct _kstore {
	const u128 kx_ncch1;
	const u128 kx_ncchA;
	const u128 kx_ncchB;
} default_keys[2] =
{
	{	/* retail */
		.kx_ncch1 = NNC_HEX128(0xCEE7D8AB30C00DAE,850EF5E382AC5AF3),
		.kx_ncchA = NNC_HEX128(0x82E9C9BEBFB8BDB8,75ECC0A07D474374),
		.kx_ncchB = NNC_HEX128(0x45AD04953992C7C8,93724A9A7BCE6182),
	},
	{	/* dev */
		.kx_ncch1 = NNC_HEX128(0x81907A4B6F1B4732,3A677974CE4AD71B),
		.kx_ncchA = NNC_HEX128(0x304BF1468372EE64,115EBD4093D84276),
		.kx_ncchB = NNC_HEX128(0x6C8B2944A0726035,F941DFC018524FB6),
	}
};

nnc_result nnc_keyset_default(nnc_keyset *ks, bool dev)
{
	const struct _kstore *s = dev ? &default_keys[1] : &default_keys[0];
	u8 type = ks->flags & TYPE_FIELD, mtype = TYPE_FLAG(dev);
	if(type && type != mtype)
		return NNC_R_MISMATCH;
	ks->kx_ncch1 = s->kx_ncch1;
	ks->kx_ncchA = s->kx_ncchA;
	ks->kx_ncchB = s->kx_ncchB;
	ks->flags |= DEFAULT | mtype;
	return NNC_R_OK;
}

static const u128 C1_b = NNC_HEX128(0x1FF9E9AAC5FE0408,024591DC5D52768A);
static const u128 *C1 = &C1_b;

static const u128 system_fixed_key = NNC_HEX128(0x527CE630A9CA305F,3696F3CDE954194B);
static const u128 fixed_key = NNC_HEX128(0x0000000000000000,0000000000000000);

static void hwkgen_3ds(u128 *output, u128 *kx, u128 *ky)
{
	*output = *kx;
	nnc_u128_rol(output, 2);
	nnc_u128_xor(output, ky);
	nnc_u128_add(output, C1);
	nnc_u128_ror(output, 41);
}

nnc_result nnc_keyy_seed(nnc_ncch_header *ncch, nnc_u128 *keyy, u8 seed[NNC_SEED_SIZE])
{
	nnc_sha256_hash hashbuf;
	nnc_u8 strbuf[0x20];
	memcpy(strbuf, seed, NNC_SEED_SIZE);
	memcpy(strbuf + NNC_SEED_SIZE, &ncch->title_id, sizeof(ncch->title_id));
	nnc_crypto_sha256(strbuf, hashbuf, 0x18);
	if(U32P(hashbuf) != ncch->seed_hash)
		return NNC_R_CORRUPT;
	nnc_u128_bytes_be(&ncch->keyy, strbuf);
	memcpy(strbuf + 0x10, seed, NNC_SEED_SIZE);
	nnc_crypto_sha256(strbuf, hashbuf, 0x20);
	*keyy = nnc_u128_import_be(hashbuf);
	return NNC_R_OK;
}

static result ky_from_ncch(u128 *ky, nnc_ncch_header *ncch, nnc_seeddb *seeddb)
{
	result ret;
	/* We need to decrypt the keyY first */
	if(ncch->flags & NNC_NCCH_USES_SEED)
	{
		if(!seeddb) return NNC_R_SEED_NOT_FOUND;
		u8 *seed;
		if(!(seed = nnc_get_seed(seeddb, ncch->title_id)))
			return NNC_R_SEED_NOT_FOUND;
		TRY(nnc_keyy_seed(ncch, ky, seed));
	}
	/* it's fine to use */
	else *ky = ncch->keyy;
	return NNC_R_OK;
}

result nnc_key_menu_info(u128 *output, nnc_keyset *ks, nnc_ncch_header *ncch,
	nnc_seeddb *seeddb)
{
	if(ncch->flags & NNC_NCCH_NO_CRYPTO)
		return NNC_R_INVAL;
	if(!(ks->flags & BOOTROM)) return NNC_R_KEY_NOT_FOUND;
	result ret; u128 ky; TRY(ky_from_ncch(&ky, ncch, seeddb));
	/* "menu info" always uses keyslot 0x2C */
	hwkgen_3ds(output, &ks->kx_ncch0, &ky);
	return NNC_R_OK;
}

result nnc_key_content(u128 *output, nnc_keyset *ks, nnc_ncch_header *ncch,
	nnc_seeddb *seeddb)
{
	if(ncch->flags & NNC_NCCH_NO_CRYPTO)
		return NNC_R_INVAL;
	if(ncch->flags & NNC_NCCH_FIXED_KEY)
	{
		*output = nnc_tid_category(ncch->title_id) & NNC_TID_CAT_SYSTEM
			? system_fixed_key : fixed_key;
		return NNC_R_OK;
	}

	result ret; u128 ky; TRY(ky_from_ncch(&ky, ncch, seeddb));

	switch(ncch->crypt_method)
	{
#define CASE(val, key, dep) case val: if(!(ks->flags & (dep))) return NNC_R_KEY_NOT_FOUND; hwkgen_3ds(output, &ks->kx_ncch##key, &ky); break;
	CASE(0x00, 0, BOOTROM)
	CASE(0x01, 1, DEFAULT)
	CASE(0x0A, A, DEFAULT)
	CASE(0x0B, B, DEFAULT)
#undef CASE
	default: return NNC_R_NOT_FOUND;
	}

	return NNC_R_OK;
}

result nnc_get_ncch_iv(struct nnc_ncch_header *ncch, u8 for_section,
	u8 counter[0x10])
{
	if(ncch->flags & NNC_NCCH_NO_CRYPTO)
		return NNC_R_INVAL;
	if(for_section < NNC_SECTION_EXTHEADER || for_section > NNC_SECTION_ROMFS)
		return NNC_R_INVAL;
	u64 *ctr64 = (u64 *) counter;
	u32 *ctr32 = (u32 *) counter;
	switch(ncch->version)
	{
	case 0:
	case 2:
		ctr64[0] = BE64(ncch->partition_id);
		ctr64[1] = 0;
		counter[8] = for_section;
		break;
	case 1:
		ctr64[0] = LE64(ncch->partition_id);
		ctr32[2] = 0;
		switch(for_section)
		{
		case NNC_SECTION_EXTHEADER:
			ctr32[3] = BE32(NNC_MEDIA_UNIT);
			break;
		case NNC_SECTION_EXEFS:
			ctr32[3] = BE32(NNC_MU_TO_BYTE(ncch->exefs_offset));
			break;
		case NNC_SECTION_ROMFS:
			ctr32[3] = BE32(NNC_MU_TO_BYTE(ncch->romfs_offset));
			break;
		}
		break;
	default:
		return NNC_R_UNSUPPORTED_VER;
	}
	return NNC_R_OK;
}

/* nnc_aes_ctr */

static void new_aes_ctr_ctx(nnc_aes_ctr *ac, u32 offset)
{
	mbedtls_aes_context *ctx = ac->crypto_ctx;
	mbedtls_aes_init(ctx);

	u8 buf[0x10];
	nnc_u128_bytes_be(&ac->key, buf);
	mbedtls_aes_setkey_enc(ctx, buf, 128);

	u128 ctr = { .hi = 0, .lo = offset / 0x10 };
	nnc_u128_add(&ctr, &ac->iv);
	nnc_u128_bytes_be(&ctr, ac->ctr);
}

static result aes_ctr_read(nnc_aes_ctr *self, u8 *buf, u32 max, u32 *totalRead)
{
	if(max % 0x10 != 0) return NNC_R_BAD_ALIGN;
	result ret;
	TRY(NNC_RS_PCALL(self->child, read, buf, max, totalRead));
	/* if we read unaligned we need to align it down */
	if(*totalRead % 0x10 != 0) *totalRead = ALIGN(*totalRead, 0x10) - 0x10;
	/* now we gotta decrypt *totalRead bytes in buf */
	u8 block[0x10];
	size_t of = 0;
	mbedtls_aes_crypt_ctr(self->crypto_ctx, *totalRead, &of, self->ctr, block, buf, buf);
	return NNC_R_OK;
}

static result aes_ctr_seek_abs(nnc_aes_ctr *self, u32 pos)
{
	if(pos % 0x10 != 0) return NNC_R_BAD_ALIGN;
	u32 cpos = NNC_RS_PCALL0(self->child, tell);
	/* i doubt this will happen but it's here anyways
	 * to save a bit of time. */
	if(cpos == pos) return NNC_R_OK;
	NNC_RS_PCALL(self->child, seek_abs, pos);
	mbedtls_aes_free(self->crypto_ctx);
	new_aes_ctr_ctx(self, pos);
	return NNC_R_OK;
}

static result aes_ctr_seek_rel(nnc_aes_ctr *self, u32 pos)
{
	if(pos % 0x10 != 0) return NNC_R_BAD_ALIGN;
	/* i doubt this will happen but it's here anyways
	 * to save a bit of time. */
	if(pos == 0) return NNC_R_OK;
	pos = NNC_RS_PCALL0(self->child, tell) + pos;
	NNC_RS_PCALL(self->child, seek_abs, pos);
	mbedtls_aes_free(self->crypto_ctx);
	new_aes_ctr_ctx(self, pos);
	return NNC_R_OK;
}

static u32 aes_ctr_size(nnc_aes_ctr *self)
{
	return NNC_RS_PCALL0(self->child, size);
}

static u32 aes_ctr_tell(nnc_aes_ctr *self)
{
	return NNC_RS_PCALL0(self->child, tell);
}

static void aes_ctr_close(nnc_aes_ctr *self)
{
	if(self->crypto_ctx)
	{
		mbedtls_aes_free(self->crypto_ctx);
		free(self->crypto_ctx);
	}
}

static const nnc_rstream_funcs aes_ctr_funcs = {
	.read = (nnc_read_func) aes_ctr_read,
	.seek_abs = (nnc_seek_abs_func) aes_ctr_seek_abs,
	.seek_rel = (nnc_seek_rel_func) aes_ctr_seek_rel,
	.size = (nnc_size_func) aes_ctr_size,
	.close = (nnc_close_func) aes_ctr_close,
	.tell = (nnc_tell_func) aes_ctr_tell,
};

nnc_result nnc_aes_ctr_open(nnc_aes_ctr *self, nnc_rstream *child, u128 *key, u8 iv[0x10])
{
	self->funcs = &aes_ctr_funcs;
	if(!(self->crypto_ctx = malloc(sizeof(mbedtls_aes_context))))
		return NNC_R_NOMEM;
	memcpy(self->ctr, iv, 0x10);
	self->iv = nnc_u128_import_be(iv);
	self->child = child;
	self->key = *key;
	new_aes_ctr_ctx(self, 0);
	return NNC_R_OK;
}

