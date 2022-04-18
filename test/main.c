
#include <nnc/read-stream.h>
#include <nnc/exefs.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdarg.h>

static void die(const char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	fprintf(stderr, "nnc-test: ");
	vfprintf(stderr, fmt, va);
	fprintf(stderr, "\n");
	va_end(va);
	exit(1);
}

int main(int argc, char *argv[])
{
	if(argc != 3) die("usage: %s <file> <output>", argv[0]);
	const char *exefs_file = argv[1];
	const char *outdir = argv[2];

	mkdir(outdir, 0777);

	nnc_file f;
	if(nnc_file_open(&f, exefs_file) != NNC_R_OK)
		die("f->open() failed");

	nnc_exefs_file_header headers[NNC_EXEFS_MAX_FILES];
	nnc_sha256_hash hashes[NNC_EXEFS_MAX_FILES];
	if(nnc_read_exefs_header(NNC_RSP(&f), headers, hashes, NULL) != NNC_R_OK)
		die("failed reading exefs file headers");

	char pathbuf[128];
	for(nnc_u8 i = 0; i < NNC_EXEFS_MAX_FILES && nnc_exefs_file_in_use(&headers[i]); ++i)
	{
		printf("%s/%s @ %08X [%08X] (", outdir, headers[i].name, headers[i].offset, headers[i].size);
		for(nnc_u8 j = 0; j < sizeof(nnc_sha256_hash); ++j)
			printf("%02X", hashes[i][j]);
		printf(nnc_verify_file(NNC_RSP(&f), headers, hashes, i) ? " HASH OK" : " NOT OK");
		printf(")\n");

		nnc_seek_exefs_file(NNC_RSP(&f), &headers[i]);
		nnc_u32 read_size;
		nnc_u8 *buf = malloc(headers[i].size);
		if(NNC_RS_CALL(f, read, buf, headers[i].size, &read_size) != NNC_R_OK || read_size != headers[i].size)
			die("failed to extract exefs file %s", headers[i].name);
		sprintf(pathbuf, "%s/%s", outdir, headers[i].name);
		FILE *ef = fopen(pathbuf, "w");
		if(fwrite(buf, headers[i].size, 1, ef) != 1)
			die("failed to write exefs file %s to %s", headers[i].name, pathbuf);
		fclose(ef);
	}

	NNC_RS_CALL(f, close);
}

