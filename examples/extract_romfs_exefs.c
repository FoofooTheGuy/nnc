/** \example extract_cia_romfs_exefs.c
 *  \brief   This example shows how to extract the RomFS and ExeFS files from NCCH.
 */

#include <nnc/stream.h>
#include <nnc/exefs.h>
#include <nnc/ncch.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	nnc_result res;
	if(argc != 4)
	{
		fprintf(stderr, "usage: %s <ncch-file> <output-romfs-file> <output-exefs-file>", argv[0]);
		return 1;
	}
	const char *ncch_file = argv[1];
	const char *romfs_file = argv[2];
	const char *exefs_file = argv[3];

	nnc_file f;
	res = nnc_file_open(&f, ncch_file);
	if (res != NNC_R_OK) {
		printf("failed to open '%s'", ncch_file);
		return 1;
	}
	
	nnc_ncch_header header;
	res = nnc_read_ncch_header(NNC_RSP(&f), &header);
	if (res != NNC_R_OK)
		printf("failed to read ncch header from '%s'", ncch_file);
	
	nnc_seeddb seeddb;
	nnc_keyset ks = NNC_KEYSET_INIT;
	res = nnc_scan_seeddb(&seeddb);
	if (res != NNC_R_OK)
		fprintf(stderr, "Failed to find a seeddb. Titles with seeds will not work.\n");
	nnc_keyset_default(&ks, NNC_KEYSET_RETAIL);
	nnc_keypair kpair;
	res = nnc_fill_keypair(&kpair, &ks, &seeddb, &header);
	if (res != NNC_R_OK)
		fprintf(stderr, "failed to fill keypair, crypto will not work.\n");
	
	nnc_ncch_section_stream romfs_stream;
	nnc_ncch_exefs_stream exefs_stream;
	
	//extract romfs
	res = nnc_ncch_section_romfs(&header, NNC_RSP(&f), &kpair, &romfs_stream);
	if (res != NNC_R_OK)
	{
		printf("failed to read romfs\n");
	}
	else {
		nnc_wfile oromfs;
		res = nnc_wfile_open(&oromfs, romfs_file);
		if (res != NNC_R_OK)
			printf("failed to create %s\n", romfs_file);
		
		res = nnc_copy(NNC_RSP(&romfs_stream), NNC_WSP(&oromfs), NULL);
		if (res != NNC_R_OK)
			printf("failed to write to %s\n", romfs_file);
		
		NNC_WS_CALL0(oromfs, close);
	}
	
	//extract exefs
	res = nnc_ncch_exefs_full_stream(&exefs_stream, &header, NNC_RSP(&f), &kpair);
	if (res != NNC_R_OK)
	{
		printf("failed to read exefs\n");
	}
	else {
		nnc_wfile oexefs;
		res = nnc_wfile_open(&oexefs, exefs_file);
		if (res != NNC_R_OK)
			printf("failed to create %s\n", exefs_file);
		
		res = nnc_copy(NNC_RSP(&exefs_stream), NNC_WSP(&oexefs), NULL);
		if (res != NNC_R_OK)
			printf("failed to write to %s\n", exefs_file);
		
		NNC_WS_CALL0(oexefs, close);
	}
	
	if (res != NNC_R_OK) {
		fprintf(stderr, "%s: failed: %s\n", argv[0], nnc_strerror(res));
		return 1;
	}
    return 0;
}
