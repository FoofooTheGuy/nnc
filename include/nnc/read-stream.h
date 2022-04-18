
#ifndef inc_nnc_read_stream_h
#define inc_nnc_read_stream_h

#include <nnc/base.h>
#include <stdio.h>
NNC_START

#define NNC_RS_PCALL(obj, func, ...) (obj)->funcs->func((struct nnc_rstream *) (obj) __VA_OPT__(,) __VA_ARGS__)
#define NNC_RS_CALL(obj, func, ...) (obj).funcs->func((struct nnc_rstream *) &(obj) __VA_OPT__(,) __VA_ARGS__)
#define NNC_RSP(obj) ((struct nnc_rstream *) (obj))

struct nnc_rstream;
typedef nnc_result (*nnc_read_func)(struct nnc_rstream *self, nnc_u8 *buf, nnc_u32 max,
		nnc_u32 *totalRead);
typedef nnc_result (*nnc_seek_abs_func)(struct nnc_rstream *self, nnc_u32 pos);
typedef nnc_result (*nnc_seek_rel_func)(struct nnc_rstream *self, nnc_u32 pos);
typedef nnc_u32 (*nnc_size_func)(struct nnc_rstream *self);
typedef void (*nnc_close_func)(struct nnc_rstream *self);
typedef struct nnc_rstream_funcs {
	nnc_read_func read;
	nnc_seek_abs_func seek_abs;
	nnc_seek_rel_func seek_rel;
	nnc_size_func size;
	nnc_close_func close;
} nnc_rstream_funcs;

typedef struct nnc_rstream {
	const nnc_rstream_funcs *funcs;
	/* user-data */
} nnc_rstream;

typedef struct nnc_file {
	const nnc_rstream_funcs *funcs;
	nnc_u32 size;
	FILE *f;
} nnc_file;

typedef struct nnc_memory {
	const nnc_rstream_funcs *funcs;
	nnc_u32 size;
	nnc_u32 pos;
	void *ptr;
} nnc_memory;

nnc_result nnc_file_open(nnc_file *self, const char *name);

NNC_END
#endif

