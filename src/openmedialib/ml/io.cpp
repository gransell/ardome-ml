#define OPENMEDIALIB_ML_IO_EXPOSE_AVIO_OPEN_CLOSE
#include "io.hpp"

#include <map>
#include <stdio.h>
#include <opencorelib/cl/assert_defines.hpp>
#include <opencorelib/cl/enforce_defines.hpp>

namespace olib { namespace openmedialib { namespace ml { namespace io {

	namespace
	{
		//Maps from protocol prefix to handler for that protocol
		typedef std::map< std::string, protocol_funcs > protocol_handler_map;

		protocol_handler_map protocol_handlers_;
		boost::recursive_mutex mutex_;

		const char *custom_io_class_prefix = "AML_CUSTOM_IO:";
	}

	void register_protocol_handler( const char *protocol_prefix, protocol_funcs funcs )
	{
		ARASSERT( protocol_prefix != NULL );
		ARASSERT( funcs.open_func != NULL );
		ARASSERT( funcs.close_func != NULL );
		ARASSERT( funcs.read_func != NULL || funcs.write_func != NULL );

		boost::recursive_mutex::scoped_lock lck( mutex_ );
		const bool insert_result =
			protocol_handlers_.insert( std::make_pair( protocol_prefix, funcs ) ).second;
		ARENFORCE_MSG( insert_result, "There was already a protocol handler registered for \"%1%\"" )
			( protocol_prefix );
	}

	int open_file( AVIOContext **out_context, const char *uri, int flags, int buffer_size )
	{
		if( ( flags & AVIO_FLAG_READ_WRITE ) == AVIO_FLAG_READ_WRITE )
		{
			//AVIO_READ_WRITE simply does not work.
			//The context can only be either read or write. Fail early to
			//save confusion later on.
			return AVERROR( EINVAL );
		}

		const char *first_colon_pos = strchr( uri, ':' );
		if( first_colon_pos != NULL )
		{
			//Check if there's a handler registered for the protocol prefix
			const protocol_funcs *handler_funcs = NULL;
			std::string prefix( uri, first_colon_pos );
			{
				boost::recursive_mutex::scoped_lock lck( mutex_ );
				protocol_handler_map::const_iterator it = protocol_handlers_.find( prefix );
				if( it != protocol_handlers_.end() )
					handler_funcs = &( it->second );
			}
			if( handler_funcs != NULL )
			{
				if( ( flags & AVIO_FLAG_READ ) && handler_funcs->read_func == NULL )
					return AVERROR(EINVAL);
				if( ( flags & AVIO_FLAG_WRITE ) && handler_funcs->write_func == NULL )
					return AVERROR(EINVAL);

				//Call protocol handler open function
				void *handler_private_data = NULL;
				int open_result = ( handler_funcs->open_func )( &handler_private_data, uri, flags );
				if( open_result != 0 )
					return open_result;

				//Allocate the AVIOContext to return, and give it the handler_private_data
				//pointer so that the protocol handler can keep state.
				unsigned char *buf = static_cast< unsigned char * >( av_malloc( buffer_size ) );
				if( buf == NULL )
					return AVERROR( ENOMEM );
				
				const int write_flag = ( flags & AVIO_FLAG_WRITE ) ? 1 : 0;

				//The cast below is a hack to be able to have a write
				//function which takes a const source buffer. For some reason,
				//avio_alloc_context wants a write function which takes a
				//modifiable source buffer.
				//(The same kind of hack is used internally in avformat for the
				//implementation of the avio_open2 function.)
				typedef int ( *avio_write_func )( void *, unsigned char *, int );
				avio_write_func wf = reinterpret_cast< avio_write_func >( handler_funcs->write_func );
				*out_context = avio_alloc_context( buf, buffer_size, write_flag,
					handler_private_data,
					handler_funcs->read_func,
					wf,
					handler_funcs->seek_func );
				if( *out_context == NULL )
				{
					av_free( buf );
					return AVERROR( ENOMEM );
				}
				ARASSERT( ( *out_context )->av_class == NULL );

				( *out_context )->seekable = ( handler_funcs->seek_func != NULL );
				( *out_context )->direct = ( flags & AVIO_FLAG_DIRECT ) ? 1 : 0;

				//Tag the av_class member of the AVIOContext with "AML_CUSTOM_IO:<prefix>",
				//so that we can recognize the custom context when closing the file and call
				//the correct close() function.
				AVClass *av_class = new AVClass();
				memset( av_class, 0, sizeof( AVClass ) );
				std::string class_name_str = custom_io_class_prefix + prefix;
				char *class_name_buf = new char[ class_name_str.size() + 1 ];
				memcpy( class_name_buf, class_name_str.c_str(), class_name_str.size() + 1 );

				av_class->class_name = class_name_buf;
				( *out_context )->av_class = av_class;

				return 0;
			}
		}

		//Found no handler, default to standard avio
		return avio_open( out_context, uri, flags );
	}

	int close_file( AVIOContext *context )
	{
		//avio_close() performs this check, so we do too
		if( context == NULL )
			return 0;

		//Make a final flush before closing.
		//avio_close does this internally, but then there's no
		//way to detect if it failed or not, since it destroys the
		//context containing the error flags right after.
		int flush_result = 0;
		if( context->write_flag )
		{
			avio_flush( context );
			flush_result = context->error;
		}

		int close_result = 0;

		ARASSERT( context->av_class != NULL );
		ARASSERT( context->av_class->class_name != NULL );
		const char *class_name = context->av_class->class_name;

		//Check for our custom prefix in the class name to know if a handler was registered for this
		if( strstr( class_name, custom_io_class_prefix ) == class_name )
		{
			std::string prefix( context->av_class->class_name + strlen( custom_io_class_prefix ) );
			void *handler_private_data = context->opaque;

			//Clean up the AVIOContext that we allocated in open_file()
			delete[] context->av_class->class_name;
			delete context->av_class;
			av_free( context->buffer );
			av_free( context );

			//Find the protocol handler close() function and call it to
			//let the handler clean up its internals
			close_func_t close_func = NULL;
			{
				boost::recursive_mutex::scoped_lock lck( mutex_ );
				protocol_handler_map::const_iterator it = protocol_handlers_.find( prefix );
				ARASSERT( it != protocol_handlers_.end() );
				close_func = it->second.close_func;
			}
			ARASSERT( close_func != NULL );
			close_result = ( close_func )( handler_private_data );
		}
		else
		{
			//No handler, which means we defaulted to avio_open2 when we opened the file.
			close_result = avio_close( context );
		}

		if( flush_result != 0 )
			return flush_result;
		else
			return close_result;
	}

} } } }

