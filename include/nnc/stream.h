/** \file   stream.h
 *  \brief  Functions relating to stream io.
 */
#ifndef inc_nnc_stream_h
#define inc_nnc_stream_h

#include <nnc/base.h>
#include <stdlib.h>
#include <stdarg.h>
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
	union nnc_memory_un {
		const void *ptr_const;
		void *ptr;
	} un;
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
void nnc_mem_open(nnc_memory *self, const void *ptr, nnc_u32 size);

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

/** \{
 *  \anchor vfs
 *  \name   Virtual FileSystem
 */

/** \brief Parameters for adding a file */
#define NNC_VFS_FILE(filename) &nnc__internal_vfs_generator_file, (filename)
/** \brief Parameters for adding a read stream */
#define NNC_VFS_READER(rs) &nnc__internal_vfs_generator_reader, (rs)

/** \brief Allocates a reader for use in a custom generator */
#define nnc_vfs_generator_allocate_reader(type) (malloc(sizeof(type)))

typedef struct nnc_vfs_generator {
	nnc_result (*reader_from_node)(nnc_rstream **out_stream, void *udata);
	nnc_result (*init_file_node)(void **out_udata, va_list params);
	void (*delete_file_node)(void *udata);
	size_t (*size)(void *udata);
} nnc_vfs_generator;

typedef struct nnc_vfs_node {
	const nnc_vfs_generator *generator;
	void *generator_data;
	char *vname;
	unsigned is_dir : 1;
} nnc_vfs_node;

typedef struct nnc_vfs {
	/* filesystems are "flat"; directory tree's are stored in one array
	 * where the directory node MUST come before any file that's inside.
	 * files are placed inside directories with <dir-name>/<file-name>.
	 * directory names MUST be read back as <dir-name>/ and must NEVER start with a slash (/) */
	nnc_vfs_node *nodes;
	int len, size;
} nnc_vfs;

/** \brief               Creates a new VFS.
 *  \param vfs           Output VFS.
 *  \param initial_size  The initial capacity for the new VFS object.
 *  \note                You must free the memory allocated by this function by a matching call to \ref nnc_vfs_free. */
nnc_result nnc_vfs_init(nnc_vfs *vfs, int initial_size);

/** \brief            Adds a new virtual file to a VFS objeca.
 *  \param vfs        VFS to add to.
 *  \param vfilename  The filename the file should have *in the VFS object*.
 *  \param generator  Usually filled out by \ref NNC_VFS_FILE or \ref NNC_VFS_READER, it is actually the generator that generates a reader (see \ref nnc_vfs_generator).
 *  \param ...        Parameters for the generator.
 *  \note             To add files in a subdirectory you must first create it with \ref nnc_vfs_add_dir.
 *  \code
 *  struct nnc_subview *my_reader;
 *  nnc_vfs_add_dir(&vfs, "some_directory/");
 *  nnc_vfs_add(&vfs, "some_directory/some_file.txt", NNC_VFS_FILE("real_dir/my_file.txt"));
 *  nnc_vfs_add(&vfs, "some_directory/some_file.bin", NNC_VFS_READER(my_reader));
 *  \endcode
 */
nnc_result nnc_vfs_add(nnc_vfs *vfs, const char *vfilename, const nnc_vfs_generator *generator, ... /* generator parameters */);

/** \brief           Adds a new virtual directory to a VFS object.
 *  \param vfs       VFS to add to.
 *  \param vdirname  Virtual directory name, with or without a final slash.
 *  \note            To link a real directory use \ref nnc_vfs_link_directory.
 */
nnc_result nnc_vfs_add_dir(nnc_vfs *vfs, const char *vdirname);

/** \brief      free()s memory in use by a VFS.
 *  \param vfs  The VFS to free
 */
void nnc_vfs_free(nnc_vfs *vfs);

/** \brief          Add all files in the real directory dirname in the VFS.
 *  \param vfs      The VFS to link into.
 *  \param dirname  The real directory to link.
 *  \param prefix   The virtual path to link into, or NULL to link into the root of the VFS.
 */
nnc_result nnc_vfs_link_directory(nnc_vfs *vfs, const char *dirname, const char *prefix);

/** \brief       Open a node in the VFS for reading.
 *  \param node  The node to open
 *  \param res   The output read stream.
 *  \note        The output read stream must be freed up with \ref nnc_vfs_close_node.
 */
nnc_result nnc_vfs_open_node(nnc_vfs_node *node, nnc_rstream **res);

/** \brief       Get the file size of a node in the VFS.
 *  \param node  Node to get size of.
 */
size_t nnc_vfs_node_size(nnc_vfs_node *node);

/** \brief     Close the read stream that is opened by \ref nnc_vfs_open_node.
 *  \param rs  The stream to close.
 */
void nnc_vfs_close_node(nnc_rstream *rs);

/* \cond INTERNAL */
extern const nnc_vfs_generator nnc__internal_vfs_generator_reader;
extern const nnc_vfs_generator nnc__internal_vfs_generator_file;
/* \endcond */

/** \} */

/** \brief       Copy the contents of a stream to another.
 *  \param from  Source read stream.
 *  \param to    Destination write stream.
 */
nnc_result nnc_copy(nnc_rstream *from, nnc_wstream *to);

NNC_END
#endif
