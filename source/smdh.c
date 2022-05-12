
#include <nnc/smdh.h>
#include <assert.h>
#include <string.h>
#include "./internal.h"


result nnc_read_smdh(rstream *rs, nnc_smdh *smdh)
{
	assert(sizeof(smdh->titles) == 0x2000 && "smdh->titles was not properly packed");

	u8 header[0x8];
	u8 settings[0x30];
	result ret;
	TRY(read_exact(rs, header, sizeof(header)));
	/* 0x0000 */ if(memcmp(header, "SMDH", 4) != 0)
	/* 0x0000 */ 	return NNC_R_CORRUPT;
	/* 0x0004 */ smdh->version = LE16P(&header[0x04]);
	/* 0x0006 */ /* reserved */
	/* 0x0008 */ TRY(read_exact(rs, (void *) smdh->titles, sizeof(smdh->titles)));
	/* 0x2008 */ TRY(read_exact(rs, settings, sizeof(settings)));
	/* 0x2008 */ memcpy(smdh->game_ratings, &settings[0x00], sizeof(smdh->game_ratings));
	/* 0x2018 */ smdh->lockout = LE32P(&settings[0x10]);
	/* 0x201C */ smdh->match_maker_id = LE32P(&settings[0x14]);
	/* 0x2020 */ smdh->match_maker_bit_id = LE64P(&settings[0x18]);
	/* 0x2028 */ smdh->flags = LE32P(&settings[0x20]);
	/* 0x202C */ smdh->eula_version_minor = settings[0x24];
	/* 0x202C */ smdh->eula_version_major = settings[0x25];
	/* 0x202E */ /* reserved */
	/* 0x2030 */ smdh->optimal_animation_frame = LE32P(&settings[0x28]);
	/* 0x2034 */ smdh->cec_id = LE32P(&settings[0x2C]);
	return NNC_R_OK;
}

result nnc_read_smdh_graphics(); // TODO

