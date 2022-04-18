
#include <nnc/read-stream.h>
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
{
	return self->size;
}

static void file_close(nnc_file *self)
{
	fclose(self->f);
}

static const nnc_rstream_funcs file_funcs = {
	.read = (nnc_read_func) file_read,
	.seek_abs = (nnc_seek_abs_func) file_seek_abs,
	.seek_rel = (nnc_seek_rel_func) file_seek_rel,
	.size = (nnc_size_func) file_size,
	.close = (nnc_close_func) file_close,
};

result nnc_file_open(nnc_file *self, const char *name)
{
	self->f = fopen(name, "r");
	if(!self->f) return NNC_R_FAIL_OPEN;

	fseek(self->f, 0, SEEK_END);
	self->size = ftell(self->f);
	fseek(self->f, 0, SEEK_SET);

	self->funcs = &file_funcs;
	return NNC_R_OK;
}

static result mem_read(nnc_memory *self, u8 *buf, u32 max, u32 *totalRead)
{
	*totalRead = MIN(max, self->size);
	memcpy(buf, self->ptr + self->pos, *totalRead);
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
	free(self->ptr);
}

static const nnc_rstream_funcs mem_funcs = {
	.read = (nnc_read_func) mem_read,
	.seek_abs = (nnc_seek_abs_func) mem_seek_abs,
	.seek_rel = (nnc_seek_rel_func) mem_seek_rel,
	.size = (nnc_size_func) mem_size,
	.close = (nnc_close_func) mem_close,
};

static const nnc_rstream_funcs mem_own_funcs = {
	.read = (nnc_read_func) mem_read,
	.seek_abs = (nnc_seek_abs_func) mem_seek_abs,
	.seek_rel = (nnc_seek_rel_func) mem_seek_rel,
	.size = (nnc_size_func) mem_size,
	.close = (nnc_close_func) mem_own_close,
};

void nnc_mem_open(nnc_memory *self, void *ptr, u32 size)
{
	self->funcs = &mem_funcs;
	self->size = size;
	self->ptr = ptr;
}

void nnc_mem_own_open(nnc_memory *self, void *ptr, u32 size)
{
	self->funcs = &mem_own_funcs;
	self->size = size;
	self->ptr = ptr;
}

