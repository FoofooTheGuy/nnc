// vim: ft=c.doxygen
/** \file  read-stream.h
 *  \brief Definitions for nnc_rstream.
 */
#ifndef inc_nnc_read_stream_h
#define inc_nnc_read_stream_h

#include <nnc/base.h>
#include <stdio.h>
NNC_START

/** \brief Call \ref nnc_rstream function on pointer. */
#define NNC_RS_PCALL(obj, func, ...) (obj)->funcs->func((struct nnc_rstream *) (obj) __VA_OPT__(,) __VA_ARGS__)
/** \brief Call \ref nnc_rstream function. */
#define NNC_RS_CALL(obj, func, ...) (obj).funcs->func((struct nnc_rstream *) &(obj) __VA_OPT__(,) __VA_ARGS__)
/** \brief Cast stream-like type to \ref nnc_rstream pointer for passing to other functions. */
#define NNC_RSP(obj) ((struct nnc_rstream *) (obj))

struct nnc_rstream;
/** Read from stream. */
typedef nnc_result (*nnc_read_func)(struct nnc_rstream *self, nnc_u8 *buf, nnc_u32 max,
		nnc_u32 *totalRead);
/** Seek to absolute position in stream. */
typedef nnc_result (*nnc_seek_abs_func)(struct nnc_rstream *self, nnc_u32 pos);
/** Seek to relative to current position in stream. */
typedef nnc_result (*nnc_seek_rel_func)(struct nnc_rstream *self, nnc_u32 pos);
/** Get total size of stream. */
typedef nnc_u32 (*nnc_size_func)(struct nnc_rstream *self);
/** Close/free the stream */
typedef void (*nnc_close_func)(struct nnc_rstream *self);

/** All functions a stream should have */
typedef struct nnc_rstream_funcs {
	nnc_read_func read;
	nnc_seek_abs_func seek_abs;
	nnc_seek_rel_func seek_rel;
	nnc_size_func size;
	nnc_close_func close;
} nnc_rstream_funcs;

/** Struct containing just a func table which should be
 *  used as parameter when taking a stream. */
typedef struct nnc_rstream {
	const nnc_rstream_funcs *funcs;
	/* user-data */
} nnc_rstream;

/** Stream for a file using the standard FILE. */
typedef struct nnc_file {
	const nnc_rstream_funcs *funcs;
	nnc_u32 size;
	FILE *f;
} nnc_file;

/** Stream for memory buffer. */
typedef struct nnc_memory {
	const nnc_rstream_funcs *funcs;
	nnc_u32 size;
	nnc_u32 pos;
	void *ptr;
} nnc_memory;

/** \brief       Create a new file stream.
 *  \param self  Output stream.
 *  \param name  Filename to open. */
nnc_result nnc_file_open(nnc_file *self, const char *name);

/** \brief       Create a new memory stream.
 *  \param self  Output stream.
 *  \param ptr   Pointer to memory.
 *  \param size  Size of memory. */
void nnc_mem_open(nnc_memory *self, void *ptr, nnc_u32 size);

/** \brief       Create a new memory stream that free()s the pointer when closed.
 *  \param self  Output stream.
 *  \param ptr   Pointer to memory.
 *  \param size  Size of memory. */
void nnc_mem_own_open(nnc_memory *self, void *ptr, nnc_u32 size);

NNC_END
#endif

