
#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/awi.hpp>

#include "index.hpp"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
}

namespace ml = olib::openmedialib;

namespace olib { namespace openmedialib { namespace ml { 

class aml_awi_index : public aml_index
{
	public:
		aml_awi_index( const char *file )
			: mutex_( )
			, file_( file )
			, awi_( )
			, position_( 0 )
		{
		}

		virtual ~aml_awi_index( )
		{
		}

		/// Read any pending data in the index
		virtual void read( )
		{
			if ( finished( ) ) return;

			URLContext *ts_context_;
			boost::uint8_t temp[ 400 ];

			if ( url_open( &ts_context_, file_.c_str( ), URL_RDONLY ) < 0 )
				return;

			url_seek( ts_context_, position_, SEEK_SET );

			while ( true )
			{
				int actual = url_read( ts_context_, ( unsigned char * )temp, sizeof( temp ) );
				if ( actual <= 0 ) break;
				position_ += actual;
				awi_.parse( temp, actual );
			}

			url_close( ts_context_ );
		}

		/// Find the byte position for the position
		virtual int64_t find( int position )
		{
			return awi_.find( position );
		}

		/// Return the number of frames as reported by the index
		/// Note that if we have not reached the eof, there is a chance that the index won't
		/// accurately reflect the number of frames in the available media (since we can't
		/// guarantee that the two files will be in sync), so prior to the reception of the 
		/// terminating record, we need to return an approximation which is > current
		virtual int frames( int current )
		{
			return awi_.frames( current );
		}

		/// Indicates if the index is complete
		virtual bool finished( )
		{
			return awi_.finished( );
		}

		/// Returns the size of the data file as indicated by the index
		virtual boost::int64_t bytes( )
		{
			return awi_.bytes( );
		}

		/// Calculate the number of available frames for the given media data size 
		/// Note that the size given can be less than, equal to or greater than the size
		/// which the index reports - any data greater than the index should be ignored, 
		/// and data less than the index should report a frame count such that the last
		/// gop can be full decoded
		virtual int calculate( boost::int64_t size )
		{
			return awi_.calculate( size );
		}

		/// Indicates if the media can use the index for byte accurate seeking or not
		/// This is a work around for audio files which don't like byte positions
		virtual bool usable( )
		{
			return awi_.usable( );
		}

		/// User of the index can indicate byte seeking usability here
		virtual void set_usable( bool value )
		{
			awi_.set_usable( value );
		}

		virtual int key_frame_of( int position )
		{
			return awi_.key_frame_of( position );
		}

		virtual int key_frame_from( boost::int64_t offset )
		{
			return awi_.key_frame_from( offset );
		}

	protected:
		boost::recursive_mutex mutex_;
		std::string file_;
		ml::awi_parser awi_;
		boost::int64_t position_;
};

class aml_file_index : public aml_index
{
	public:
		aml_file_index( const char *file )
			: mutex_( )
			, file_( file )
			, position_( 0 )
			, eof_( false )
			, frames_( 0 )
			, usable_( true )
		{
		}

		virtual ~aml_file_index( )
		{
		}

		virtual void read( )
		{
			if ( eof_ ) return;

			FILE *fd = fopen( file_.c_str( ), "r" );
			
			if ( fd )
			{
				char temp[ 132 ];
				fseek( fd, position_, SEEK_SET );
				while( true )
				{
					position_ = ftell( fd );
					if ( fgets( temp, 132, fd ) && temp[ strlen( temp ) - 1 ] == '\n' )
					{
						int index = 0;
						boost::int64_t position;
						if ( sscanf( temp, "%d=%lld", &index, &position ) == 2 )
						{
							boost::recursive_mutex::scoped_lock lock( mutex_ );
							key_index_[ index ] = position;
							off_index_[ position ] = index;
						}
						else if ( sscanf( temp, "eof:%d=%lld", &index, &position ) == 2 )
						{
							boost::recursive_mutex::scoped_lock lock( mutex_ );
							key_index_[ index ] = position;
							off_index_[ position ] = index;
							eof_ = true;
						}
						else
						{
							//std::cerr << "Invalid index info " << temp << std::endl;
						}
						{
							boost::recursive_mutex::scoped_lock lock( mutex_ );
							if ( index > frames_ )
								frames_ = index;
						}
					}
					else
					{
						break;
					}
				}

				fclose( fd );
			}
		}

		bool finished( )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			return eof_;
		}

		int64_t find( int position )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			boost::int64_t byte = -1;

			if ( key_index_.size( ) )
			{
				std::map< int, boost::int64_t >::iterator iter = key_index_.upper_bound( position );
				if ( iter != key_index_.begin( ) ) iter --;
				byte = ( *iter ).second;
			}

			return byte;
		}

		int frames( int current )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			int result = frames_;

			// If the current value does not indicate that the index and data are in sync, then we want
			// to return a value which is greater than the current, and significantly less than the value
			// indicated by the index 
			if ( current < frames_ )
			{
				std::map< int, boost::int64_t >::iterator iter = key_index_.upper_bound( frames_ - 100 );
				if ( iter != key_index_.begin( ) ) iter --;
				result = ( *iter ).first >= current ? ( *iter ).first + 1 : current + 1;
			}

			return result;
		}

		boost::int64_t bytes( )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			std::map< int, boost::int64_t >::iterator iter = -- key_index_.end( );
			return ( *iter ).second;
		}

		int calculate( boost::int64_t size )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			int result = -1;
			if ( off_index_.size( ) )
			{
				std::map< boost::int64_t, int >::iterator iter = off_index_.upper_bound( size );
				if ( iter == off_index_.end( ) ) iter --;
				result = ( *iter ).second;
			}
			return result;
		}

		bool usable( )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			return usable_;
		}

		void set_usable( bool value )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			usable_ = value;
		}

		virtual int key_frame_of( int position )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			int result = -1;

			if ( key_index_.size( ) )
			{
				std::map< int, boost::int64_t >::iterator iter = key_index_.upper_bound( position );
				if ( iter != key_index_.begin( ) ) iter --;
				result = ( *iter ).first;
			}
			return result;
		}

		// Return the key frame associated to the offset
		virtual int key_frame_from( boost::int64_t offset )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			std::map< boost::int64_t, int >::iterator iter = off_index_.upper_bound( offset );
			if ( iter != off_index_.begin( ) ) iter --;
			return ( *iter ).second;
		}

	protected:
		boost::recursive_mutex mutex_;
		std::string file_;
		int position_;
		std::map < int, boost::int64_t > key_index_;
		std::map < boost::int64_t, int > off_index_;
		bool eof_;
		int frames_;
		bool usable_;
};

class aml_http_index : public aml_file_index
{
	public:
		aml_http_index( const char *file )
			: aml_file_index( file )
			, remainder_ ( "" )
		{
		}

		void read( )
		{
			if ( eof_ ) return;

			char temp[ 400 ];

			if ( url_open( &ts_context_, file_.c_str( ), URL_RDONLY ) < 0 )
			{
				usable_ = false;
				eof_ = true;
				return;
			}

			url_seek( ts_context_, position_, SEEK_SET );

			while ( true )
			{
				int actual = 0;
				int bytes = remainder_.length( );
				int requested = 200 - bytes;
				char *ptr = temp;

				memset( temp, 0, sizeof( temp ) );
				memcpy( temp, remainder_.c_str( ), bytes );

				actual = url_read( ts_context_, ( unsigned char * )temp + bytes, requested );
				if ( actual <= 0 ) break;

				while( ptr && strchr( ptr, '\n' ) )
				{
					int index = 0;
					boost::int64_t position;
					if ( sscanf( ptr, "%d=%lld", &index, &position ) == 2 )
					{
						boost::recursive_mutex::scoped_lock lock( mutex_ );
						key_index_[ index ] = position;
						off_index_[ position ] = index;
						if ( index > frames_ )
							frames_ = index;
					}
					else if ( !strncmp( ptr, "eof:", 4 ) && sscanf( ptr + 4, "%d=%lld", &index, &position ) == 2 )
					{
						boost::recursive_mutex::scoped_lock lock( mutex_ );
						key_index_[ index ] = position;
						off_index_[ position ] = index;
						eof_ = true;
						if ( index > frames_ )
							frames_ = index;
					}
					else
					{
						//std::cerr << "Invalid index info " << ptr << std::endl;
					}

					ptr = strchr( ptr, '\n' );
					if ( ptr ) ptr ++;
				}

				if ( ptr )
					remainder_ = ptr;
				else
					remainder_ = "";
			}

			position_ = url_seek( ts_context_, 0, SEEK_CUR );
			url_close( ts_context_ );
		}

	private:
		URLContext *ts_context_;
		std::string remainder_;
};

aml_index *aml_index_factory( const char *url )
{
	aml_index *result = 0;

	// Check for new version (binary AWI v2) first
	if ( strstr( url, ".awi" ) )
	{
		result = new aml_awi_index( url );
		result->read( );

		if ( result->frames( 0 ) == 0 )
		{
			delete result;
			result = 0;
		}
	}

	// Fallback to text based indexing
	if ( result == 0 )
	{
		result = new aml_http_index( url );

		if ( result )
		{
			result->read( );
			if ( result->frames( 0 ) == 0 )
			{
				delete result;
				result = 0;
			}
		}
	}

	return result;
}

} } }

