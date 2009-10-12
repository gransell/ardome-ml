#// ml - A media library representation.

// Copyright (C) 2009 Ardendo
// Released under the LGPL.

#include <openmedialib/ml/indexer.hpp>
#include <openmedialib/ml/packet.hpp>
#include <openmedialib/ml/awi.hpp>
#include <openmedialib/ml/input.hpp>
#include <openmedialib/ml/frame.hpp>
#include <openmedialib/ml/audio.hpp>
#include <openmedialib/ml/utilities.hpp>

// includes for openpluginlib string handling
#include <openpluginlib/pl/utf8_utils.hpp>

// includes for opencorelib worker
#include <opencorelib/cl/core.hpp>
#include <opencorelib/cl/enforce_defines.hpp>
#include <opencorelib/cl/function_job.hpp>
#include <opencorelib/cl/worker.hpp>

// includes for access to the avformat url access interface
extern "C" {
#include <libavformat/avformat.h>
}

namespace pl = olib::openpluginlib;

namespace olib { namespace openmedialib { namespace ml {

// Decorates indexer item with job control functionality
class indexer_job : public indexer_item
{
	public:
		virtual ~indexer_job( ) { }
		virtual void job_request( opencorelib::base_job_ptr ) = 0;
		virtual const boost::posix_time::milliseconds job_delay( ) const = 0;
};

// Shared pointer for the indexer_job
typedef ML_DECLSPEC boost::shared_ptr< indexer_job > indexer_job_ptr;

// Forward declaration to local factory method
static indexer_job_ptr indexer_job_factory( const pl::wstring &url );

// Possibly redundant?
class ML_DECLSPEC indexer_type
{
	public:
		/// Obtain the item associated to the url
		virtual void init( ) = 0;
		virtual indexer_item_ptr request( const openpluginlib::wstring &url ) = 0;
		virtual void shutdown( ) = 0;
};

// Possibly redundant?
typedef ML_DECLSPEC boost::shared_ptr< indexer_type > indexer_type_ptr;

// The indexer singleton
class indexer : public indexer_type
{
	public:
		/// Provides the singleton instance of this object
		static indexer_type_ptr instance( )
		{
			static indexer_type_ptr instance_( new indexer( ) );
			return instance_;
		}

		/// Destructor of the singleton
		~indexer( )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			index_read_worker_.stop( boost::posix_time::seconds( 5 ) );
		}

		// Force an initialisation of the worker
		void init( )
		{
		}

		/// Request an index item for the specified url
		indexer_item_ptr request( const pl::wstring &url )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			if ( map_.find( url ) == map_.end( ) )
			{
				indexer_job_ptr job = indexer_job_factory( url );
				if ( !job->finished( ) )
				{
					opencorelib::function_job_ptr read_job = opencorelib::function_job_ptr( new opencorelib::function_job( boost::bind( &indexer_job::job_request, job, _1 ) ) );
					index_read_worker_.add_reoccurring_job( read_job, job->job_delay( ) );
				}
				map_[ url ] = job;
			}
			return map_[ url ];
		}

		void shutdown( )
		{
			index_read_worker_.stop( boost::posix_time::seconds( 5 ) );
		}

	private:
		/// Constructor for the indexer collection
		indexer( )
		{
			index_read_worker_.start( );
		}

		mutable boost::recursive_mutex mutex_;
		std::map< pl::wstring, indexer_job_ptr > map_;
		opencorelib::worker index_read_worker_;
};

/// Index reading template
template < class T > class aml_index_reader : public T
{
	public:
		aml_index_reader( const std::string &indexname )
			: T( )
			, file_( indexname )
			, position_( 0 )
		{
			read( );
		}

		virtual ~aml_index_reader( )
		{
		}

		/// Read any pending data in the index
		void read( )
		{
			if ( T::finished( ) ) return;
	
			URLContext *ts_context;
			boost::uint8_t temp[ 16384 ];
	
			if ( url_open( &ts_context, file_.c_str( ), URL_RDONLY ) < 0 )
				return;
	
			url_seek( ts_context, position_, SEEK_SET );
	
			while ( true )
			{
				int actual = url_read( ts_context, ( unsigned char * )temp, sizeof( temp ) );
				if ( actual <= 0 ) break;
				position_ += actual;
				T::parse( temp, actual );
			}
	
			url_close( ts_context );
		}

	protected:
		std::string file_;
		boost::int64_t position_;
};

// Shared pointer wrapper for index reader
typedef ML_DECLSPEC boost::shared_ptr< awi_index > aml_index_reader_ptr;

// Factory method for creating the index reader
aml_index_reader_ptr create( const std::string& indexname )
{
	aml_index_reader_ptr result;

	// Try V3 Index
	result = aml_index_reader_ptr( new aml_index_reader< awi_parser_v3 >( indexname ) );
	if ( result->frames( 0 ) == 0 )
		result = aml_index_reader_ptr( );

	if ( !result )
	{
		// Try V2 Index
		result = aml_index_reader_ptr( new aml_index_reader< awi_parser_v2 >( indexname ) );
		if ( result->frames( 0 ) == 0 )
			result = aml_index_reader_ptr( );
	}

	return result;
}

class indexed_job_type : public indexer_job 
{
	public:
		indexed_job_type( const pl::wstring &url, aml_index_reader_ptr index )
			: url_( url )
			, index_( index )
		{
			index_->read( );
		}

		awi_index_ptr index( )
		{
			return index_;
		}

		const boost::int64_t size( ) const
		{
			return index_->bytes( );
		}

		const bool finished( ) const
		{
			return index_->finished( );
		}

		void job_request( opencorelib::base_job_ptr job )
		{
			index_->read( );
			job->set_should_reschedule( !finished( ) );
		}

		const boost::posix_time::milliseconds job_delay( ) const
		{
			return boost::posix_time::milliseconds( 1000 );
		}

	private:
		pl::wstring url_;
		aml_index_reader_ptr index_;
};

class unindexed_job_type : public indexer_job 
{
	public:
		unindexed_job_type( const pl::wstring &url )
			: url_( url )
			, size_( 0 )
			, finished_( false )
		{
			check_size( );
		}

		awi_index_ptr index( )
		{
			return awi_index_ptr( );
		}

		const boost::int64_t size( ) const
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			return size_;
		}

		const bool finished( ) const
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			return false;
		}

		void job_request( opencorelib::base_job_ptr job )
		{
			check_size( );
			job->set_should_reschedule( !finished( ) );
		}

		const boost::posix_time::milliseconds job_delay( ) const
		{
			return boost::posix_time::milliseconds( 2000 );
		}

	private:
		void check_size( )
		{
			if ( finished( ) ) return;

			URLContext *context = 0;
			std::string temp = pl::to_string( url_ );

			// Attempt to reopen the media
			if ( url_open( &context, temp.c_str( ), URL_RDONLY ) >= 0 )
			{
				// Check the current file size
				boost::int64_t bytes = url_seek( context, 0, SEEK_END );

				// Clean up the temporary sizing context
				url_close( context );

				// If it's larger than before, then reopen, otherwise reduce resize count
				{
					boost::recursive_mutex::scoped_lock lock( mutex_ );
					if ( bytes > size_ )
						size_ = bytes;
					else
						finished_ = true;
				}
			}
		}

	private:
		mutable boost::recursive_mutex mutex_;
		pl::wstring url_;
		boost::int64_t size_;
		bool finished_;
};

static const pl::pcos::key key_offset_ = pl::pcos::key::from_string( "offset" );
static const pl::pcos::key key_file_size_ = pl::pcos::key::from_string( "file_size" );

class generating_job_type : public indexer_job 
{
	public:
		generating_job_type( const pl::wstring &url )
			: url_( url )
			, index_( awi_generator_v3_ptr( new awi_generator_v3( ) ) )
			, input_( create_input( url ) )
			, start_( 0 )
		{
			if ( input_ )
			{
				last_frame_ = input_->fetch( 0 );
				if ( last_frame_ && last_frame_->get_stream( ) )
				{
					index_->enroll( last_frame_->get_position( ), last_frame_->get_stream( )->properties( ).get_property_with_key( key_offset_ ).value< boost::int64_t >( ) );
					start_ += 1;
					analyse_gop( );
				}
			}
			else
			{
			}
		}

		awi_index_ptr index( )
		{
			return index_;
		}

		const boost::int64_t size( ) const
		{
			return index_->bytes( );
		}

		const bool finished( ) const
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			return input_ && input_->get_frames( ) > 0 && index_->finished( );
		}

		void job_request( opencorelib::base_job_ptr job )
		{
			analyse_gop( );
			job->set_should_reschedule( !finished( ) );
		}

		const boost::posix_time::milliseconds job_delay( ) const
		{
			return boost::posix_time::milliseconds( 300 );
		}


	private:
		void analyse_gop( )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			while ( input_ && start_ < input_->get_frames( ) )
			{
				last_frame_ = input_->fetch( start_ ++ );
				if ( last_frame_->get_stream( )->position( ) == last_frame_->get_stream( )->key( ) )
				{
					index_->enroll( last_frame_->get_position( ), last_frame_->get_stream( )->properties( ).get_property_with_key( key_offset_ ).value< boost::int64_t >( ) );
					break;
				}
			}

			if ( last_frame_ && start_ == input_->get_frames( ) )
				index_->close( last_frame_->get_position( ), input_->properties( ).get_property_with_key( key_file_size_ ).value< boost::int64_t >( ) );
		}

		mutable boost::recursive_mutex mutex_;
		pl::wstring url_;
		awi_generator_v3_ptr index_;
		input_type_ptr input_;
		int start_;
		frame_type_ptr last_frame_;
};

static indexer_job_ptr indexer_job_factory( const pl::wstring &url )
{
	if ( url.find( L"index:" ) != 0 )
	{
		aml_index_reader_ptr index = create( pl::to_string( url ) );
		if ( index )
			return indexer_job_ptr( new indexed_job_type( url, index ) );
		else
			return indexer_job_ptr( new unindexed_job_type( url ) );
	}
	else
	{
		return indexer_job_ptr( new generating_job_type( url ) );
	}
}

void ML_DECLSPEC indexer_init( )
{
	return indexer::instance( )->init( );
}

indexer_item_ptr ML_DECLSPEC indexer_request( const openpluginlib::wstring &url )
{
	return indexer::instance( )->request( url );
}

void ML_DECLSPEC indexer_shutdown( )
{
	return indexer::instance( )->shutdown( );
}

} } }
