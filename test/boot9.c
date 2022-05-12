
#include <nnc/read-stream.h>
#include <nnc/crypto.h>
#include <string.h>
#include <stdio.h>

void die(const char *fmt, ...);


int boot9_main(int argc, char *argv[])
{
	if(argc < 2) die("usage: %s <boot9.bin> <is_dev ('yes', 'no' or omitted for no)>", argv[0]);
	const char *boot9_file = argv[1];
	bool is_dev = false;
	if(argc > 2)
	{
		if(strcmp(argv[2], "yes") == 0)
			is_dev = true;
		else if(strcmp(argv[2], "no") != 0)
			die("is_dev was neither 'yes', 'no' or omitted");
	}

	nnc_file boot9;
	if(nnc_file_open(&boot9, boot9_file) != NNC_R_OK)
		die("failed to open '%s'", boot9_file);

	nnc_keyset ks = NNC_KEYSET_INIT;
	if(nnc_keyset_boot9(NNC_RSP(&boot9), &ks, is_dev) != NNC_R_OK)
		die("failed to read keys from '%s'", boot9_file);

#define DUMP(var, slot, type) printf(#slot " " #type "=" NNC_FMT128_LOWER "\n", NNC_ARG128(ks.var))
	DUMP(kx_ncch0, 0x2C, keyX);
#undef DUMP

	NNC_RS_CALL0(boot9, close);
	return 0;
}

