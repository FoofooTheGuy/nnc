
#include <nnc/read-stream.h>
#include <nnc/romfs.h>
#include <nnc/utf.h>
#include <stdlib.h>
#include <stdio.h>

void die(const char *fmt, ...);


static void print_dir(nnc_romfs_ctx *ctx, nnc_romfs_info *dir, int indent)
{
	nnc_romfs_iterator it = nnc_romfs_mkit(ctx, dir);
	nnc_romfs_info ent;
	while(nnc_romfs_next(&it, &ent))
	{
		int len = ent.filename_length * 4;
		char *fname = malloc(len + 1);
		fname[nnc_utf16_to_utf8((nnc_u8 *) fname, len, ent.filename, ent.filename_length)] = '\0';
		printf("%*s%s%s\n", indent, "", fname, ent.type == NNC_ROMFS_DIR ? "/" : "");
		free(fname);
		if(ent.type == NNC_ROMFS_DIR)
			print_dir(ctx, &ent, indent + 1);
	}
}

int romfs_main(int argc, char *argv[])
{
	if(argc != 2) die("usage: %s <file>", argv[0]);
	const char *romfs_file = argv[1];

	nnc_file f;
	if(nnc_file_open(&f, romfs_file) != NNC_R_OK)
		die("f->open() failed");

	nnc_result res;
	nnc_romfs_ctx ctx;
	if((res = nnc_init_romfs(NNC_RSP(&f), &ctx)) != NNC_R_OK)
		die("nnc_init_romfs() failed");

	printf(
		"== %s ==\n"
		" == RomFS header ==\n"
		"  Directory Hash Offset      : 0x%lX\n"
		"                 Length      : 0x%X\n"
		"  Directory Metadata Offset  : 0x%lX\n"
		"                     Length  : 0x%X\n"
		"  File Hash Offset           : 0x%lX\n"
		"            Length           : 0x%X\n"
		"  File Metadata Offset       : 0x%lX\n"
		"                Length       : 0x%X\n"
		"  Data Offset                : 0x%lX\n"
		" == RomFS files & directories ==\n"
	, romfs_file, ctx.header.dir_hash.offset
	, ctx.header.dir_hash.length, ctx.header.dir_meta.offset
	, ctx.header.dir_meta.length, ctx.header.file_hash.offset
	, ctx.header.file_hash.length, ctx.header.file_meta.offset
	, ctx.header.file_meta.length, ctx.header.data_offset);

	nnc_romfs_info info;
	if(nnc_get_info(&ctx, &info, "/") != NNC_R_OK)
		die("failed root directory info");
	print_dir(&ctx, &info, 2);

	nnc_free_romfs(&ctx);

	NNC_RS_CALL0(f, close);
	return 0;
}

