#ifndef OPENMEDIALIB_ML_IO_HPP_INCLUDED
#define OPENMEDIALIB_ML_IO_HPP_INCLUDED

#include <openmedialib/ml/types.hpp>

extern "C"
{
	#include <libavformat/avio.h>
}

namespace olib { namespace openmedialib { namespace ml { namespace io {

	extern "C"
	{
		/// Called when file_open() is called on a URI containing the prefix for the handler.
		/**
		@param out_private_data Optional output parameter. A handler implementation may store a
		pointer to internal data in this parameter. That pointer will then be sent as the
		private_data parameter in all calls to the other handler functions.
		@param uri The URI sent to file_open(). Includes the protocol prefix for the handler.
		@param flags The flags sent to file_open().
		@return If succesful, 0 shall be returned. If unsuccesful, then an appropriate AVERROR
			code.
		*/
		typedef int ( *open_func_t )( void **out_private_data, const char *uri, int flags );

		/// Called when close_file() is called on the AVIOContext associated with the handler.
		/**
		The handler is responsible for freeing any resources associated with the instance when
		this function is called.
		@param private_data The pointer stored by the handler in out_private_data when open_func()
			 was called. If no pointer was stored at that time, the value is NULL.
		@return If successful, 0 shall be returned. If unsuccessful, then an appropriate AVERROR
			code.
		*/
		typedef int ( *close_func_t )( void *private_data );

		/// Read a maximum of size bytes from the file into buf.
		/**
		@param private_data The pointer stored by the handler in out_private_data when open_func()
			 was called. If no pointer was stored at that time, the value is NULL.
		@param buf The buffer where the read bytes are stored.
		@param size The desired number of bytes to read.
		@return If successful, the number of bytes read (may be less than size). If unsuccesful,
			then an appropriate AVERROR code.
		*/
		typedef int ( *read_func_t )( void *private_data, unsigned char *buf, int size );

		/// Write a maximum of size bytes from buf into the file.
		/**
		@param private_data The pointer stored by the handler in out_private_data when open_func()
			 was called. If no pointer was stored at that time, the value is NULL.
		@param buf The buffer containing the bytes to be written.
		@param size The desired number of bytes to write.
		@return If successful, the number of bytes written (may be less than size). If unsuccesful,
			then an appropriate AVERROR code.
		*/
		typedef int ( *write_func_t )( void *private_data, const unsigned char *buf, int size );

		/// Seek to the given offset in the file.
		/**
		@param private_data The pointer stored by the handler in out_private_data when open_func()
			 was called. If no pointer was stored at that time, the value is NULL.
		@param offset The offset to seek to within the file.
		@param whence Specifies how the offset parameter is to be interpreted:
			SEEK_SET: The offset is relative to the start of the file.
			SEEK_CUR: The offset is relative to the current file offset.
			SEEK_END: The offset is relative to the end of the file.
			AVSEEK_SIZE: Do not perform a seek, but instead return the file size.
			Handler implementations are required to support SEEK_SET, SEEK_CUR and SEEK_END,
			but are strongly encouraged to also support AVSEEK_SIZE.
			Note: If using avio_seek() in the internal handler implementation, bear in mind that
			it only supports SEEK_SET and SEEK_CUR, so you will need to handle the SEEK_END and
			AVSEEK_SIZE cases manually.
		*/
		typedef boost::int64_t ( *seek_func_t )( void *private_data, boost::int64_t offset, int whence );
	}

	/// Structure containing the functions that implement a custom protocol handler.
	/**
	open_func: may not be NULL
	close_func: may not be NULL
	seek_func: may be NULL for non-seekable I/O
	read_func: may be NULL for write-only I/O
	write_func: may be NULL for read-only I/O
	*/
	struct protocol_funcs
	{
		open_func_t open_func;
		close_func_t close_func;
		read_func_t read_func;
		write_func_t write_func;
		seek_func_t seek_func;
	};

	/// Registers a handler for a custom I/O protocol.
	/**
	A protocol is used by prefixing the URI to open with the protocol prefix followed by a colon.
	For example, a URI using the protocol "custom" would look like:
	custom:http://server/file.wav
	custom:file://dir/file.wav
	This function registers a handler for a custom I/O protocol, so that URI:s containing the
	prefix may be opened via the open_file() function.
	@param protocol_prefix A string containing the protocol prefix, without the ending colon.
	@param funcs A structure containing pointers to the custom functions that will be called
		when a URI with the custom prefix is opened, and when the resulting AVIOContext object
		is accessed. See the documentation for the handler function signatures for details.
	*/
	void ML_DECLSPEC register_protocol_handler( const char *protocol_prefix, protocol_funcs funcs );

	/// Opens a file from a URI with support for custom I/O protocols.
	/**
	Opens the file pointed to by the uri parameter. If the uri starts with a protocol
	previously registered through register_protocol_handler(), the corresponding
	open function for that protocol will be called. If not, it defaults to calling
	avio_open().
	Important: Always use the close_file() function to close the context returned
		from this function; never use avio_close().
	@param out_context Output parameter. If the call was successful, it will contain
		a pointer to an AVIOContext structure. This context is suitable for use
		with the avio_* group of methods (except avio_close(), see above).
	@param uri The URI of the file to open.
	@param flags One or more of the AVIO_FLAG_* constants, controlling how the file
		is to be opened:
		AVIO_FLAG_READ: read-only mode
		AVIO_FLAG_WRITE: write-only mode
		Important: Do not use AVIO_FLAG_READ_WRITE. Despite this flag's existence, an
		avio context can only be opened in either read or write mode. Due to this,
		open_file() will fail with code AVERROR(EINVAL) if sent this parameter.
	@param buffer_size The buffer size to use. Has no effect if the URI does not
		reference a custom protocol. In that case, the buffer size is automatically
		decided by avio.
	@return 0 on success, an AVERROR code on failure.
	*/
	int ML_DECLSPEC open_file( AVIOContext **out_context, const char *uri, int flags, int buffer_size = 32768 );

	/// Closes a file previously opened with open_file().
	/**
	@param context A pointer to an AVIOContext returned from a previous call
		to open_file().
	@return 0 on success, an AVERROR code on failure. For files opened for writing,
		an AVERROR code could indicate either a failure to flush the file write or a
		failure in the closing itself. Thus, to ensure the integrity of written data, be
		sure to check the return value.
	*/
	int ML_DECLSPEC close_file( AVIOContext *context );

	//Client code should use open_file and close_file above, so that URI:s with
	//custom protocols are handled.
#ifndef OPENMEDIALIB_ML_IO_EXPOSE_AVIO_OPEN_CLOSE
	#define avio_open use_open_file_from_openmedialib_ml_io_hpp_instead_of_avio_open
	#define avio_close use_close_file_from_openmedialib_ml_io_hpp_instead_of_avio_close
#endif

} } } }

#endif

