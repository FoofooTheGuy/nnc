// vim: ft=c.doxygen
#ifndef inc_nnc_exefs_h
#define inc_nnc_exefs_h
/** \file  exefs.h
 *  \brief Functions relating to exefs.
 *  \see   https://www.3dbrew.org/wiki/ExeFS */
#include <nnc/read-stream.h>
#include <nnc/crypto.h>
#include <nnc/base.h>
NNC_START

/** Maximum files an exefs has. */
#define NNC_EXEFS_MAX_FILES 10

/** Exefs file header. */
typedef struct nnc_exefs_file_header {
	char name[9];    /** Filename, NULL-terminated. */
	nnc_u32 offset;  /** Offset in exefs. */
	nnc_u32 size;    /** File size. */
} nnc_exefs_file_header;

/** Returns whether or not a file header is used. */
bool nnc_exefs_file_in_use(nnc_exefs_file_header *fh);

/** \brief          Read the header of an exefs.
 *  \param rs       Stream to read exefs from.
 *  \param headers  (optional) Array of size \ref NNC_EXEFS_MAX_FILES.
 *  \param hashes   (optional) Array of size \ref NNC_EXEFS_MAX_FILES.
 *  \param size     (optional) Amount of used files. Only set if headers is not NULL.
 */
nnc_result nnc_read_exefs_header(nnc_rstream *rs, nnc_exefs_file_header *headers,
	nnc_sha256_hash *hashes, nnc_u8 *size);

/** \brief          Searches the exefs for \p name.
 *  \return         Returns the appropriate exefs header index of -1 if none was found.
 *  \param name     Name to search for. Maximum length is 8.
 *  \param headers  Array of size \ref NNC_EXEFS_MAX_FILES returned by \ref nnc_read_exefs_header.
 */
nnc_i8 nnc_find_exefs_file_index(const char *name, nnc_exefs_file_header *headers);

/** \brief         Seek to the data offset of a file header.
 *  \param rs      Stream to seek in.
 *  \param header  File to seek to
 */
void nnc_seek_exefs_file(nnc_rstream *rs, nnc_exefs_file_header *header);

/** \brief          Verify if an exefs file is good.
 *  \param rs       Stream to read exefs from.
 *  \param headers  All headers.
 *  \param hashes   All hashes.
 *  \param i        Index of file to verify.
 */
bool nnc_verify_file(nnc_rstream *rs, nnc_exefs_file_header *headers,
	nnc_sha256_hash *hashes, nnc_u8 i);

NNC_END
#endif
