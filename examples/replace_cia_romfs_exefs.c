/** \example replace_cia_romfs.c
 *  \brief   This example shows how to rebuild a CIA with a different RomFS.
 */

#include <nnc/cia.h>
#include <nnc/ticket.h>
#include <stdio.h>

#define TRYB(expr, lbl) if((res = ( expr )) != NNC_R_OK) goto lbl

int main(int argc, char *argv[])
{
    if(argc != 6)
    {
        fprintf(stderr, "usage: %s <cia-file> <replacement-romfs-dir> <replacement-exefs-dir> <replacement-exheader-file> <output-cia-file>\n", argv[0]);
        return 1;
    }

    nnc_subview certchain, ticket, tmd_strm, logo;
    nnc_buildable_ncch ncch0b;
    nnc_tmd_header tmd;
    nnc_cia_writable_ncch ncch0;
    nnc_ncch_header ncch_hdr;
    nnc_cia_content_reader reader;
    nnc_cia_content_stream ncch0stream;
    nnc_file exheader;
    nnc_cia_header cia_hdr;
    nnc_result res;
    nnc_wfile wf;
    nnc_file f;
    nnc_vfs romfs;
    nnc_vfs exefs;
    nnc_keypair kp;
    nnc_seeddb sdb;

    TRYB(nnc_file_open(&f, argv[1]), out1); /* open the input file */
    TRYB(nnc_wfile_open(&wf, argv[5]), out2); /* open the output file */
    TRYB(nnc_read_cia_header(NNC_RSP(&f), &cia_hdr), out3); /* read the cia header */
    nnc_cia_open_certchain(&cia_hdr, NNC_RSP(&f), &certchain); /* open the certificate chain for later copying it into the new cia */
    nnc_cia_open_ticket(&cia_hdr, NNC_RSP(&f), &ticket); /* open the ticket for later copying it into the new cia */
    nnc_cia_open_tmd(&cia_hdr, NNC_RSP(&f), &tmd_strm); /* open the tmd which we will modify some things of and then write tot he new cia */
    TRYB(nnc_read_tmd_header(NNC_RSP(&tmd_strm), &tmd), out3); /* parse the ticket */
    TRYB(nnc_cia_make_reader(&cia_hdr, NNC_RSP(&f), nnc_get_default_keyset(), &reader), out3); /* create a content (= NCCH) reader */
    TRYB(nnc_cia_open_content(&reader, 0, &ncch0stream, NULL), out4); /* open the first content (NCCH0) */
    TRYB(nnc_read_ncch_header(NNC_RSP(&ncch0stream), &ncch_hdr), out5); /* parse the NCCH header */
    TRYB(nnc_vfs_init(&romfs), out5); /* initialize a VFS */
    TRYB(nnc_vfs_link_directory(&romfs.root_directory, argv[2], nnc_vfs_identity_transform, NULL), out6); /* populate the VFS, another source of files could be a RomFS, see #nnc_romfs_to_vfs */
    TRYB(nnc_vfs_init(&exefs), out5); /* initialize a VFS */
    TRYB(nnc_vfs_link_directory(&exefs.root_directory, argv[3], nnc_vfs_identity_transform, NULL), out10);
    if((res = nnc_scan_seeddb(&sdb)) != NNC_R_OK) /* scan for a seeddb for use with "new crypto" and set it as the default */
        fprintf(stderr, "%s: failed to load seeddb: %s\n", argv[0], nnc_strerror(res));
    nnc_set_default_seeddb(&sdb);
    TRYB(nnc_fill_keypair(&kp, nnc_get_default_keyset(), nnc_get_default_seeddb(), &ncch_hdr), out7); /* generate the cryptographic keys for if the NCCH is encrypted */
    //TRYB(nnc_ncch_section_exheader(&ncch_hdr, NNC_RSP(&ncch0stream), &kp, &exheader), out9); /* exheader stream, note that this is usually present in any content that's not 0 */
	if(nnc_file_open(&exheader, argv[4]) != NNC_R_OK) {
		printf("failed to open '%s'", argv[4]);
		goto out10;
	}
	nnc_exheader exhdr;
	if(nnc_read_exheader(NNC_RSP(&exheader), &exhdr) != NNC_R_OK) {
		printf("failed to read exheader from '%s'", argv[4]);
		goto out10;
	}
    TRYB(nnc_ncch_section_logo(&ncch_hdr, NNC_RSP(&ncch0stream), &logo), out9); /* logo stream */
    //TRYB(nnc_ncch_section_plain(&ncch_hdr, NNC_RSP(&ncch0stream), &plain), out9); /* plain stream */
    /* An excersise left for the reader: the logo, exheader, exefs and plain (and romfs) sections are optional, for more robust code check if
     * the result returned is NNC_R_NOT_FOUND and don't add said section if it is. */

    /* setup the parameters for building, for more options see the documentation. */
    ncch0.type = NNC_CIA_NCCHBUILD_BUILD;
    ncch0.ncch = &ncch0b;
    nnc_condense_ncch(&ncch0b.chdr, &ncch_hdr);
    ncch0b.wflags = NNC_NCCH_WF_ROMFS_VFS | NNC_NCCH_WF_EXEFS_VFS | NNC_NCCH_WF_EXHEADER_STREAM;
    ncch0b.romfs = &romfs;
    ncch0b.exefs = &exefs;
    ncch0b.exheader = &exheader;
    ncch0b.logo = NNC_RSP(&logo);
    ncch0b.plain = NULL;
	ncch0b.chdr.partition_id = exhdr.title_id;
	ncch0b.chdr.title_id = exhdr.title_id;
    tmd.content_count = 1;
	tmd.title_id = exhdr.title_id;
	/*nnc_ticket tik;
	if(nnc_read_ticket(&ticket, &tik) != NNC_R_OK) {
		puts("failed to read ticket");
		goto out2;
	}
	printf("%016lX\n", tik.title_id);*/
	//replace ticket title ID
	char *ticket_contents = malloc(NNC_RS_CALL0(ticket, size));
	nnc_u32 out_size = 0;
	if(NNC_RS_CALL(ticket, read, ticket_contents, NNC_RS_CALL0(ticket, size), &out_size) != NNC_R_OK)
		goto out11;
	if(out_size != NNC_RS_CALL0(ticket, size))
		goto out11;
	nnc_memory modified_ticket;
	*(nnc_u64*) &ticket_contents[nnc_sig_dsize(ticket_contents[3]) + 0xDC] = __builtin_bswap64(exhdr.title_id);
	nnc_mem_open(&modified_ticket, ticket_contents, NNC_RS_CALL0(ticket, size));
	
    /* and finally write the cia */
    res = nnc_write_cia(
        NNC_CIA_WF_CERTCHAIN_STREAM | NNC_CIA_WF_TICKET_STREAM | NNC_CIA_WF_TMD_BUILD,
        &certchain, &modified_ticket, &tmd, 1, &ncch0, NNC_WSP(&wf)
    );
    /* cleanup code, with lots of labels to jump to in case of failure depending on where it failed */
out11: free(ticket_contents);
out10: nnc_vfs_free(&exefs);
out9: NNC_RS_CALL0(exheader, close);
//out8: NNC_RS_CALL0(exefs, close);
out7: nnc_free_seeddb(&sdb);
out6: nnc_vfs_free(&romfs);
out5: NNC_RS_CALL0(ncch0stream, close);
out4: nnc_cia_free_reader(&reader);
out3: NNC_WS_CALL0(wf, close);
out2: NNC_RS_CALL0(f, close);
out1:
    if(res != NNC_R_OK)
    {
        fprintf(stderr, "%s: failed: %s\n", argv[0], nnc_strerror(res));
        return 1;
    }
    return 0;
}
