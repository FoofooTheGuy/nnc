
#ifndef inc_nnc_nncpp_stream_hh
#define inc_nnc_nncpp_stream_hh

#include <nncpp/base.hh>
#include <nnc/stream.h>
#include <string>


namespace nnc
{
	class read_stream_like
	{
	public:
		template <typename TStream = nnc_rstream>
		TStream *as_rstream()
		{
			return (TStream *) this->as_rstream_impl();
		}

	protected:
		virtual nnc_rstream *as_rstream_impl() = 0;
	};

	template <typename CStreamType>
	class c_read_stream : public read_stream_like
	{
	public:
		c_read_stream(CStreamType& cstream)
			: stream(cstream) { }
		c_read_stream() { }

		~c_read_stream()
		{
			this->close();
		}

		void close()
		{
			if(this->is_open_p)
			{
				this->as_rstream()->funcs->close(this->as_rstream());
				this->is_open_p = false;
			}
		}

		template <size_t N>
		result read(byte_array<N>& barr, u32& totalRead)
		{
			return this->read(barr.data(), barr.size(), totalRead);
		}

		result read(void *buf, u32 max, u32& totalRead)
		{
			if(!this->is_open_p) return nnc::result::not_open;
			return (nnc::result) this->as_rstream()->funcs->read(this->as_rstream(), (u8 *) buf, max, &totalRead);
		}

		result seek_abs(u32 pos)
		{
			if(!this->is_open_p) return nnc::result::not_open;
			return (nnc::result) this->as_rstream()->funcs->seek_abs(this->as_rstream(), pos);
		}

		result seek_rel(u32 offset)
		{
			if(!this->is_open_p) return nnc::result::not_open;
			return (nnc::result) this->as_rstream()->funcs->seek_rel(this->as_rstream(), offset);
		}

		u32 tell()
		{
			if(!this->is_open_p) return 0;
			return this->as_rstream()->funcs->tell(this->as_rstream());
		}

		u32 size()
		{
			if(!this->is_open_p) return 0;
			return this->as_rstream()->funcs->size(this->as_rstream());
		}

		void set_open_state(bool state)
		{
			this->is_open_p = state;
		}

		bool is_open()
		{
			return this->is_open_p;
		}

	protected:
		bool is_open_p = false;
		CStreamType stream;

		nnc_rstream *as_rstream_impl() override
		{
			return (nnc_rstream *) &this->stream;
		}

	};

	class file final : public c_read_stream<nnc_file>
	{
	public:
		using c_read_stream::c_read_stream;

#if NNCPP_ALLOW_IGNORE_ERRORS
		file(const std::string& filename) { this->open(filename); }
		file(const char *filename) { this->open(filename); }
#endif

		result open(const std::string& filename) { return this->open(filename.c_str()); }

		result open(const char *filename)
		{
			/* ensure the file is closed */
			this->close();
			result ret = (result) nnc_file_open(&this->stream, filename);
			if(ret == nnc::result::ok)
				this->is_open_p = true;
			return ret;
		}
	};

	class subview final : public c_read_stream<nnc_subview>
	{
	public:
		using c_read_stream::c_read_stream;
		subview(nnc_rstream *child, u32 offset, u32 len)
		{
			this->open(child, offset, len);
		}

		subview(read_stream_like& child, u32 offset, u32 len)
		{
			this->open(child, offset, len);
		}

		void open(read_stream_like& child, u32 offset, u32 len)
		{
			this->open(child.as_rstream(), offset, len);
		}

		void open(nnc_rstream *child, u32 offset, u32 len)
		{
			this->close();
			nnc_subview_open(&this->stream, child, offset, len);
			this->is_open_p = true;
		}
	};

	class memory final : public c_read_stream<nnc_memory>
	{
	public:
		using c_read_stream::c_read_stream;
		memory(const void *ptr, u32 size)
		{
			this->open(ptr, size);
		}

		template <size_t N>
		memory(byte_array<N>& barr)
		{
			this->open(barr);
		}

		template <size_t N>
		void open(byte_array<N>& barr)
		{
			this->open(barr.data(), N);
		}

		void open(const void *ptr, u32 size)
		{
			nnc_mem_open(&this->stream, ptr, size);
			this->is_open_p = true;
		}
	};

	/* interface for a custom read stream */
	class read_stream : public read_stream_like
	{
	private:
		struct wrapper_rstream {
			const nnc_rstream_funcs *funcs;
			read_stream *self;
		};

		static cresult c_read(nnc_rstream *obj, u8 *buf, u32 max, u32 *totalRead) { return (cresult) ((wrapper_rstream *) obj)->self->read(buf, max, *totalRead); }
		static cresult c_seek_abs(nnc_rstream *obj, u32 pos) { return (cresult) ((wrapper_rstream *) obj)->self->seek_abs(pos); }
		static cresult c_seek_rel(nnc_rstream *obj, u32 offset) { return (cresult) ((wrapper_rstream *) obj)->self->seek_rel(offset); }
		static u32 c_size(nnc_rstream *obj) { return ((wrapper_rstream *) obj)->self->size(); }
		static void c_close(nnc_rstream *obj) { ((wrapper_rstream *) obj)->self->close(); }
		static u32 c_tell(nnc_rstream *obj) { return ((wrapper_rstream *) obj)->self->tell(); }

		static constexpr nnc_rstream_funcs c_funcs = {
			c_read, c_seek_abs, c_seek_rel,
			c_size, c_close, c_tell,
		};

	public:
		virtual result read(u8 *buf, u32 max, u32& totalRead) = 0;
		virtual result seek_abs(u32 pos) = 0;
		virtual result seek_rel(u32 offset) = 0;
		virtual u32 size() = 0;
		virtual void close() = 0;
		virtual u32 tell() = 0;

	protected:
		nnc_rstream *as_rstream_impl() override
		{
			return (nnc_rstream *) &this->stream;
		}

	private:
 		wrapper_rstream stream = { &c_funcs, this };

	};
}

#endif

