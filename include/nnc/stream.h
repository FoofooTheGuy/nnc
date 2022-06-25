#ifndef inc_nnc_stream_h
#define inc_nnc_stream_h

#include <nnc/base.h>
#include <stdio.h>
NNC_BEGIN

/** \{
 *  \anchor read
 *  \name   Read Stream
 */

/** \brief Call \ref nnc_rstream function on pointer. */
#define NNC_RS_PCALL(obj, func, ...) ((nnc_rstream *) obj)->funcs->func((nnc_rstream *) (obj), __VA_ARGS__)
/** \brief Call \ref nnc_rstream function. */
#define NNC_RS_CALL(obj, func, ...) NNC_RS_PCALL(&obj, func, __VA_ARGS__)
/** \brief Cast stream-like type to \ref nnc_rstream pointer for passing to other functions. */
#define NNC_RSP(obj) ((nnc_rstream *) (obj))
/** \brief Call a \ref nnc_rstream function without arguments on a pointer. */
#define NNC_RS_PCALL0(obj, func) ((nnc_rstream *) obj)->funcs->func((nnc_rstream *) (obj))
/** \brief Call a \ref nnc_rstream function without arguments. */
#define NNC_RS_CALL0(obj, func) NNC_RS_PCALL0(&obj, func)

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
/** Get current position in stream */
typedef nnc_u32(*nnc_tell_func)(struct nnc_rstream *self);

/** All functions a stream should have */
typedef struct nnc_rstream_funcs {
	nnc_read_func read;
	nnc_seek_abs_func seek_abs;
	nnc_seek_rel_func seek_rel;
	nnc_size_func size;
	nnc_close_func close;
	nnc_tell_func tell;
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

/** Stream for reading a specific part of another stream */
typedef struct nnc_subview {
	const nnc_rstream_funcs *funcs;
	nnc_rstream *child;
	nnc_u32 size;
	nnc_u32 off;
	nnc_u32 pos;
} nnc_subview;

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

/** \brief        Create a new subview stream.
 *  \param self   Output stream.
 *  \param child  Child stream.
 *  \param off    Starting offset in \p child.
 *  \param len    Length of data in \p child.
 *  \note         Closing this stream has no effect; the child stream is not closed.
 */
void nnc_subview_open(nnc_subview *self, nnc_rstream *child, nnc_u32 off, nnc_u32 len);

/** \} */

/** \{
 *  \anchor write
 *  \name   Write Stream
 */

/** \brief Call \ref nnc_wstream function on pointer. */
#define NNC_WS_PCALL(obj, func, ...) ((nnc_wstream *) obj)->funcs->func((nnc_wstream *) (obj), __VA_ARGS__)
/** \brief Call \ref nnc_wstream function. */
#define NNC_WS_CALL(obj, func, ...) NNC_WS_PCALL(&obj, func, __VA_ARGS__)
/** \brief Cast stream-like type to \ref nnc_wstream pointer for passing to other functions. */
#define NNC_WSP(obj) ((nnc_wstream *) (obj))
/** \brief Call a \ref nnc_wstream function without arguments on a pointer. */
#define NNC_WS_PCALL0(obj, func) ((nnc_wstream *) obj)->funcs->func((nnc_wstream *) (obj))
/** \brief Call a \ref nnc_wstream function without arguments. */
#define NNC_WS_CALL0(obj, func) NNC_WS_PCALL0(&obj, func)

struct nnc_wstream;
typedef nnc_result (*nnc_write_func)(struct nnc_wstream *self, nnc_u8 *buf, nnc_u32 size);
typedef void (*nnc_wclose_func)(struct nnc_wstream *self);

typedef struct nnc_wstream_funcs {
	nnc_write_func write;
	nnc_wclose_func close;
} nnc_wstream_funcs;

typedef struct nnc_wstream {
	const nnc_wstream_funcs *funcs;
	/* user-data */
} nnc_wstream;

typedef struct nnc_wfile {
	const nnc_wstream_funcs *funcs;
	FILE *f;
} nnc_wfile;

nnc_result nnc_wfile_open(nnc_wfile *self, const char *name);


/** \} */

nnc_result nnc_copy(nnc_rstream *from, nnc_wstream *to);

NNC_END
#endif


