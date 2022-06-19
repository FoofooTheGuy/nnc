/** \file  write-stream.h
 *  \brief Definitions for nnc_wstream.
 */
#ifndef inc_nnc_write_stream_h
#define inc_nnc_write_stream_h

#include <nnc/base.h>
#include <stdio.h>
NNC_BEGIN

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

NNC_END
#endif

