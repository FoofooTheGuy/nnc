
#define _DEFAULT_SOURCE
#define _BSD_SOURCE

#include <nnc/stream.h>
#include <stdlib.h>
#include <string.h>
#include "./internal.h"

static result file_read(nnc_file *self, u8 *buf, u32 max, u32 *totalRead)
{
	u32 total = fread(buf, 1, max, self->f);
	if(totalRead) *totalRead = total;
	if(total != max)
		return ferror(self->f) ? NNC_R_FAIL_READ : NNC_R_OK;
	return NNC_R_OK;
}

static result file_seek_abs(nnc_file *self, u32 pos)
{
	if(pos >= self->size) return NNC_R_SEEK_RANGE;
	fseek(self->f, pos, SEEK_SET);
	return NNC_R_OK;
}

static result file_seek_rel(nnc_file *self, u32 pos)
{
	u32 npos = ftell(self->f) + pos;
	if(npos >= self->size) return NNC_R_SEEK_RANGE;
	fseek(self->f, npos, SEEK_SET);
	return NNC_R_OK;
}

static u32 file_size(nnc_file *self)
{ return self->size; }

static void file_close(nnc_file *self)
{ fclose(self->f); }

static u32 file_tell(nnc_file *self)
{ return ftell(self->f); }

static const nnc_rstream_funcs file_funcs = {
	.read = (nnc_read_func) file_read,
	.seek_abs = (nnc_seek_abs_func) file_seek_abs,
	.seek_rel = (nnc_seek_rel_func) file_seek_rel,
	.size = (nnc_size_func) file_size,
	.close = (nnc_close_func) file_close,
	.tell = (nnc_tell_func) file_tell,
};

result nnc_file_open(nnc_file *self, const char *name)
{
	self->f = fopen(name, "rb");
	if(!self->f) return NNC_R_FAIL_OPEN;

	fseek(self->f, 0, SEEK_END);
	self->size = ftell(self->f);
	fseek(self->f, 0, SEEK_SET);

	self->funcs = &file_funcs;
	return NNC_R_OK;
}

static nnc_result wfile_write(nnc_wfile *self, nnc_u8 *buf, nnc_u32 size)
{ return fwrite(buf, 1, size, self->f) == size ? NNC_R_OK : NNC_R_FAIL_WRITE; }

static void wfile_close(nnc_wfile *self)
{ fclose(self->f); }

static const nnc_wstream_funcs wfile_funcs = {
	.write = (nnc_write_func) wfile_write,
	.close = (nnc_wclose_func) wfile_close,
};

result nnc_wfile_open(nnc_wfile *self, const char *name)
{
	self->f = fopen(name, "wb");
	if(!self->f) return NNC_R_FAIL_OPEN;
	self->funcs = &wfile_funcs;
	return NNC_R_OK;
}

static result mem_read(nnc_memory *self, u8 *buf, u32 max, u32 *totalRead)
{
	*totalRead = MIN(max, self->size);
	memcpy(buf, ((u8 *) self->un.ptr_const) + self->pos, *totalRead);
	self->pos += *totalRead;
	return NNC_R_OK;
}

static result mem_seek_abs(nnc_memory *self, u32 pos)
{
	if(pos >= self->size) return NNC_R_SEEK_RANGE;
	self->pos = pos;
	return NNC_R_OK;
}

static result mem_seek_rel(nnc_memory *self, u32 pos)
{
	u32 npos = self->pos + pos;
	if(npos >= self->size) return NNC_R_SEEK_RANGE;
	self->pos = npos;
	return NNC_R_OK;
}

static u32 mem_size(nnc_memory *self)
{
	return self->size;
}

static void mem_close(nnc_memory *self)
{
	/* nothing to do ... */
	(void) self;
}

static void mem_own_close(nnc_memory *self)
{
	free(self->un.ptr);
}

static u32 mem_tell(nnc_memory *self)
{
	return self->pos;
}

static const nnc_rstream_funcs mem_funcs = {
	.read = (nnc_read_func) mem_read,
	.seek_abs = (nnc_seek_abs_func) mem_seek_abs,
	.seek_rel = (nnc_seek_rel_func) mem_seek_rel,
	.size = (nnc_size_func) mem_size,
	.close = (nnc_close_func) mem_close,
	.tell = (nnc_tell_func) mem_tell,
};

static const nnc_rstream_funcs mem_own_funcs = {
	.read = (nnc_read_func) mem_read,
	.seek_abs = (nnc_seek_abs_func) mem_seek_abs,
	.seek_rel = (nnc_seek_rel_func) mem_seek_rel,
	.size = (nnc_size_func) mem_size,
	.close = (nnc_close_func) mem_own_close,
	.tell = (nnc_tell_func) mem_tell,
};

void nnc_mem_open(nnc_memory *self, const void *ptr, u32 size)
{
	self->funcs = &mem_funcs;
	self->size = size;
	self->un.ptr_const = ptr;
	self->pos = 0;
}

void nnc_mem_own_open(nnc_memory *self, void *ptr, u32 size)
{
	self->funcs = &mem_own_funcs;
	self->size = size;
	self->un.ptr = ptr;
	self->pos = 0;
}

static result subview_read(nnc_subview *self, u8 *buf, u32 max, u32 *totalRead)
{
	u32 sizeleft = self->size - self->pos;
	max = MIN(max, sizeleft);
	result ret;
	/* seek to correct offset in child */
	TRY(NNC_RS_PCALL(self->child, seek_abs, self->off + self->pos));
	ret = NNC_RS_PCALL(self->child, read, buf, max, totalRead);
	self->pos += *totalRead;
	return ret;
}

static result subview_seek_abs(nnc_subview *self, u32 pos)
{
	if(pos >= self->size) return NNC_R_SEEK_RANGE;
	self->pos = pos;
	return NNC_R_OK;
}

static result subview_seek_rel(nnc_subview *self, u32 pos)
{
	u32 npos = self->pos + pos;
	if(npos >= self->size) return NNC_R_SEEK_RANGE;
	self->pos = npos;
	return NNC_R_OK;
}

static u32 subview_size(nnc_subview *self)
{
	return self->size;
}

static void subview_close(nnc_subview *self)
{
	/* nothing to do... */
	(void) self;
}

static nnc_u32 subview_tell(nnc_subview *self)
{
	return self->pos;
}

static const nnc_rstream_funcs subview_funcs = {
	.read = (nnc_read_func) subview_read,
	.seek_abs = (nnc_seek_abs_func) subview_seek_abs,
	.seek_rel = (nnc_seek_rel_func) subview_seek_rel,
	.size = (nnc_size_func) subview_size,
	.close = (nnc_close_func) subview_close,
	.tell = (nnc_tell_func) subview_tell,
};

void nnc_subview_open(nnc_subview *self, nnc_rstream *child, nnc_u32 off, nnc_u32 len)
{
	self->funcs = &subview_funcs;
	self->child = child;
	self->size = len;
	self->off = off;
	self->pos = 0;
}

/* ...vfs code... */

static char *copy_vname(const char *vname, bool is_dir)
{
	char *ret = malloc(strlen(vname) + 1), *retpos;
	const char *pos, *slash;
	if(!ret) return NULL;
	int len;
	retpos = ret;
	pos = vname;
	/* now copy vname path segment per path segment, so that we may skip any double slashes */
	do {
		while(*pos == '/')
			++pos;
		slash = strchr(pos, '/');
		if(!slash) slash = pos + strlen(pos);
		len = slash - pos;
		memcpy(retpos, pos, len);
		retpos[len] = '/';
		retpos += len + 1;
		pos = slash;
	} while(*slash);

	/* the final slash should be trimmed instead of just terminated */
	if(!is_dir && retpos != pos)
		--retpos;
	*retpos = '\0';

	return ret;
}

nnc_result nnc_vfs_init(nnc_vfs *vfs, int initial_size)
{
	/* no real rationale for making this 32 other than it looks nice */
	if(!initial_size) initial_size = 32;

	vfs->size = initial_size;
	vfs->len = 0;

	return (vfs->nodes = malloc(initial_size * sizeof(nnc_vfs_node))) ? NNC_R_OK : NNC_R_NOMEM;
}

static struct nnc_vfs_node *nnc_vfs_allocate_node(nnc_vfs *vfs)
{
	if(vfs->size == vfs->len)
	{
		int newsize = vfs->size * 2;
		nnc_vfs_node *nnodes = realloc(vfs->nodes, newsize * sizeof(nnc_vfs_node));
		if(!nnodes) return NULL;
		vfs->nodes = nnodes;
		vfs->size = newsize;
	}
	return &vfs->nodes[vfs->len++];
}

nnc_result nnc_vfs_add(nnc_vfs *vfs, const char *vfilename, const nnc_vfs_generator *generator, ... /* generator parameters */)
{
	nnc_vfs_node *node;
	if(!(node = nnc_vfs_allocate_node(vfs)))
		return NNC_R_NOMEM;

	node->vname = copy_vname(vfilename, false);
	node->generator = generator;
	node->is_dir = 0;

	if(!node->vname)
	{
		/* de-allocate the node */
		--vfs->len;
		return NNC_R_NOMEM;
	}

	va_list va;
	va_start(va, generator);
	nnc_result ret = generator->init_file_node(&node->generator_data, va);
	va_end(va);

	return ret;
}

nnc_result nnc_vfs_add_dir(nnc_vfs *vfs, const char *vdirname)
{
	nnc_vfs_node *node;
	if(!(node = nnc_vfs_allocate_node(vfs)))
		return NNC_R_NOMEM;

	node->is_dir = 1;
	node->generator = NULL;
	node->generator_data = NULL;
	node->vname = copy_vname(vdirname, true);

	if(!node->vname)
	{
		--vfs->len;
		return NNC_R_NOMEM;
	}

	return NNC_R_OK;
}

void nnc_vfs_free(nnc_vfs *vfs)
{
	nnc_vfs_node *node;
	for(int i = 0; i < vfs->len; ++i)
	{
		node = &vfs->nodes[i];
		if(node->generator)
			node->generator->delete_file_node(node->generator_data);
		free(node->vname);
	}
	free(vfs->nodes);
}

nnc_result nnc_vfs_open_node(nnc_vfs_node *node, nnc_rstream **res)
{
	return node->generator->reader_from_node(res, node->generator_data);
}

size_t nnc_vfs_node_size(nnc_vfs_node *node)
{
	return node->generator->size(node->generator_data);
}

void nnc_vfs_close_node(nnc_rstream *rs)
{
	NNC_RS_PCALL0(rs, close);
	free(rs);
}

#if NNC_PLATFORM_UNIX || NNC_PLATFORM_3DS
	#include <sys/types.h>
	#include <dirent.h>
	#define DIRENT_API 1
#elif NNC_PLATFORM_WINDOWS
	#include <windows.h>
	#define WINDOWS_API 1
#endif

#if DIRENT_API /* || WINDOWS_API */
static nnc_result nnc_vfs_link_directory_impl(nnc_vfs *vfs, const char *dirname, const char *reldir)
{
	size_t namealloc = 0, prefixalloc = strlen(dirname) + 1, nlen, vnamealloc = 0, vprefixalloc = strlen(reldir) + 2;
	char *name = malloc(prefixalloc), *vname = NULL;
	nnc_result ret;
	if(!name) return NNC_R_NOMEM;
	vname = malloc(vprefixalloc);
	if(!vname) { ret = NNC_R_NOMEM; goto fail; }
	size_t off = sprintf(name, "%s%s", dirname, dirname[prefixalloc - 2] == '/' ? "" : "/");
	size_t voff = sprintf(vname, "%s%s", reldir, reldir[0] ? "/" : "");
	DIR *d = opendir(name);

	if(!d) return NNC_R_FAIL_OPEN;

	struct dirent *ent;
	while((ent = readdir(d)))
	{
		if(strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
			continue;

		nlen = strlen(ent->d_name) + 1;
#define DOAPPEND(varprefix) \
	if(nlen > varprefix##namealloc)  \
	{ \
		char *nptr = realloc(varprefix##name, varprefix##prefixalloc + nlen); \
		varprefix##namealloc = nlen; \
		if(!nptr) \
		{ \
			ret = NNC_R_NOMEM; \
			goto fail; \
		} \
		varprefix##name = nptr; \
	} \
	memcpy(varprefix##name + varprefix##off, ent->d_name, nlen)

		DOAPPEND(   );
		DOAPPEND( v );
#undef DOAPPEND

		/* d_type should probably be made optional... they are glibc extensions after all */
		if(ent->d_type == DT_DIR)
		{
			TRYLBL(nnc_vfs_add_dir(vfs, vname), fail);
			TRYLBL(nnc_vfs_link_directory_impl(vfs, name, vname), fail);
		}
		else if(ent->d_type == DT_REG)
		{
			TRYLBL(nnc_vfs_add(vfs, vname, NNC_VFS_FILE(name)), fail);
		}
	}

fail:
	free(name);
	free(vname);
	closedir(d);
	return ret;
}
#else
static nnc_result nnc_vfs_link_directory_impl(nnc_vfs *vfs, const char *dirname, const char *ndir)
{
	(void) vfs;
	(void) dirname;
	(void) ndir;
	return NNC_R_UNSUPPORTED;
}
#endif

nnc_result nnc_vfs_link_directory(nnc_vfs *vfs, const char *dirname, const char *prefix)
{
	return nnc_vfs_link_directory_impl(vfs, dirname, prefix ? prefix : "");
}

struct nnc_filegen_data {
	char realname[1]; /* in reality this element is larger than 1, blame GCC complaining about the struct not having named members */
};

static nnc_result nnc_filegen_reader_from_node(nnc_rstream **out_stream, void *udata)
{
	struct nnc_filegen_data *data = (struct nnc_filegen_data *) udata;

	struct nnc_file *f = nnc_vfs_generator_allocate_reader(nnc_file);
	if(!f) return NNC_R_NOMEM;

	*out_stream = (nnc_rstream *) f;
	return nnc_file_open(f, data->realname);
}

static nnc_result nnc_filegen_init_file_node(void **out_udata, va_list params)
{
	const char *fname = va_arg(params, const char *);
	struct nnc_filegen_data	*udata = (struct nnc_filegen_data *) nnc_strdup(fname);
	if(!udata) return NNC_R_NOMEM;
	*out_udata = udata;
	return NNC_R_OK;
}

static void nnc_filegen_delete_file_node(void *udata)
{
	free(udata);
}

static size_t nnc_filegen_size(void *udata)
{
	struct nnc_filegen_data *data = (struct nnc_filegen_data *) udata;

	(void) data;

	return 0;
}

const nnc_vfs_generator nnc__internal_vfs_generator_file = {
	.reader_from_node = nnc_filegen_reader_from_node,
	.init_file_node = nnc_filegen_init_file_node,
	.delete_file_node = nnc_filegen_delete_file_node,
	.size = nnc_filegen_size,
};

static nnc_result nnc_sgen_reader_from_node(nnc_rstream **out_stream, void *udata)
{
	nnc_rstream *rs = (nnc_rstream *) udata;
	/* XXX: Should we seek to 0 here? */
	*out_stream = rs;
	return NNC_R_OK;
}

static nnc_result nnc_sgen_init_file_node(void **out_udata, va_list params)
{
	nnc_rstream *rs = va_arg(params, nnc_rstream *);
	*out_udata = rs;
	return NNC_R_OK;
}

static size_t nnc_sgen_size(void *udata)
{
	nnc_rstream *rs = (nnc_rstream *) udata;
	return NNC_RS_PCALL0(rs, size);
}

const nnc_vfs_generator nnc__internal_vfs_generator_reader = {
	.reader_from_node = nnc_sgen_reader_from_node,
	.init_file_node = nnc_sgen_init_file_node,
	.delete_file_node = NULL,
	.size = nnc_sgen_size,
};

//

nnc_result nnc_copy(nnc_rstream *from, nnc_wstream *to)
{
	u8 block[BLOCK_SZ];
	u32 left = NNC_RS_PCALL0(from, size), next, actual;
	result ret;
	TRY(NNC_RS_PCALL(from, seek_abs, 0));

	while(left != 0)
	{
		next = MIN(left, BLOCK_SZ);
		TRY(NNC_RS_PCALL(from, read, block, next, &actual));
		if(actual != next) return NNC_R_TOO_SMALL;
		TRY(NNC_WS_PCALL(to, write, block, next));
		left -= next;
	}
	return NNC_R_OK;
}

