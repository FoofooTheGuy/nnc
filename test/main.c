
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define DIE_USAGE() die("usage: [ dump-bootrom-keys | extract-exefs | extract-romfs | romfs-info | ncch-info | tmd-info | smdh-info | test-u128 ]")

static const char *opt = "nnc-test";
void die(const char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	fprintf(stderr, "%s: ", opt);
	vfprintf(stderr, fmt, va);
	fprintf(stderr, "\n");
	va_end(va);
	exit(1);
}

void print_hash(unsigned char *b)
{
	for(int i = 0; i < 0x20; ++i)
		printf("%02X", b[i]);
}


int extract_exefs_main(int argc, char *argv[]); /* extract-exefs.c */
int ncch_info_main(int argc, char *argv[]); /* ncch.c */
int tmd_info_main(int argc, char *argv[]); /* tmd-info.c */
int xromfs_main(int argc, char *argv[]); /* romfs.c */
int romfs_main(int argc, char *argv[]); /* romfs.c */
int boot9_main(int argc, char *argv[]); /* boot9.c */
int smdh_main(int argc, char *argv[]); /* smdh.c */
int u128_main(int argc, char *argv[]); /* u128.c */

int main(int argc, char *argv[])
{
	if(argc < 2) DIE_USAGE();
	const char *cmd = argv[1];
	argv[1] = argv[0];
	--argc;
#define CASE(cmdn, func) if(strcmp(cmd, cmdn) == 0) do { opt = "nnc-test: " cmdn; return func(argc, &argv[1]); } while(0)
	CASE("dump-bootrom-keys", boot9_main);
	CASE("extract-exefs", extract_exefs_main);
	CASE("extract-romfs", xromfs_main);
	CASE("ncch-info", ncch_info_main);
	CASE("romfs-info", romfs_main);
	CASE("tmd-info", tmd_info_main);
	CASE("smdh-info", smdh_main);
	CASE("test-u128", u128_main);
#undef CASE
	DIE_USAGE();
}

