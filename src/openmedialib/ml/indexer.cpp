#// ml - A media library representation.

// Copyright (C) 2009 Ardendo
// Released under the LGPL.

#include <openmedialib/ml/ml.hpp>
#include <openmedialib/ml/indexer.hpp>
#include <openmedialib/ml/stream.hpp>
#include <openmedialib/ml/awi.hpp>
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
#include <libavformat/url.h>
}

namespace pl = olib::openpluginlib;

namespace olib { namespace openmedialib { namespace ml {

static pl::pcos::key key_complete_ = pl::pcos::key::from_string( "complete" );

// Decorates indexer item with job control functionality
class indexer_job : public indexer_item, public opencorelib::function_job
{
	public:
		indexer_job()
		: opencorelib::function_job( boost::bind( &indexer_job::job_request, this ) )
		{ }

		virtual ~indexer_job( ) { }
		virtual void job_request( ) = 0;
		virtual const boost::posix_time::milliseconds job_delay( ) const = 0;
};

// Shared pointer for the indexer_job
typedef ML_DECLSPEC boost::shared_ptr< indexer_job > indexer_job_ptr;

// Forward declaration to local factory method
static indexer_job_ptr indexer_job_factory( const pl::wstring &url, boost::uint16_t v4_index_entry_type = 0 );

template < class T >
class aml_index_reader;

template < class T >
void aml_index_read_impl( aml_index_reader<T> *target );

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

		virtual ~aml_index_reader( ) {}

		void read( )
		{
			aml_index_read_impl< T >( this );
		}

		friend void aml_index_read_impl<T>( aml_index_reader<T> *target );

	protected:
		std::string file_;
		boost::int64_t position_;
};

/// Index reading template
template < > class aml_index_reader< awi_parser_v4 > : public awi_parser_v4
{
	public:
		aml_index_reader( const std::string &indexname, boost::uint16_t index_entry_type )
			: awi_parser_v4( index_entry_type )
			, file_( indexname )
			, position_( 0 )
		{
			read( );
		}

		virtual ~aml_index_reader( ) {}
		
		void read( )
		{
			aml_index_read_impl< awi_parser_v4 >( this );
		}	

		friend void aml_index_read_impl< awi_parser_v4 >( aml_index_reader<awi_parser_v4> *target );

	protected:
		std::string file_;
		boost::int64_t position_;
};

/// Helper function for the aml_index_reader
/// Read any pending data in the index
template < class T >
void aml_index_read_impl( aml_index_reader<T> *target )
{
	if ( target->T::finished( ) ) return;

	URLContext *ts_context;
	boost::uint8_t temp[ 16384 ];

	if ( ffurl_open( &ts_context, target->file_.c_str( ), AVIO_FLAG_READ, 0, 0 ) < 0 )
	{
		ARLOG_ERR( "ffurl_open failed on index file %1%" )( target->file_ );
		return;
	}

	ffurl_seek( ts_context, target->position_, SEEK_SET );

	while ( target->T::valid( ) )
	{
		int actual = ffurl_read( ts_context, ( unsigned char * )temp, int( sizeof( temp ) ) );
		if ( actual <= 0 ) break;
		target->position_ += actual;
		target->T::parse( temp, actual );
	}

	ffurl_close( ts_context );
}

// Shared pointer wrapper for index reader
typedef ML_DECLSPEC boost::shared_ptr< awi_index > aml_index_reader_ptr;

// Factory method for creating the index reader
aml_index_reader_ptr create( const std::string& indexname, boost::uint16_t v4_index_entry_type = 0 )
{
	aml_index_reader_ptr result;

	if ( v4_index_entry_type != 0 )
	{
		// Try V4 Index
		result = aml_index_reader_ptr( new aml_index_reader< awi_parser_v4 >( indexname, v4_index_entry_type ) );
		if ( result->total_frames( ) == 0 )
			result = aml_index_reader_ptr( );
	}

	if ( !result )
	{
		// Try V3 Index
		result = aml_index_reader_ptr( new aml_index_reader< awi_parser_v3 >( indexname ) );
		if ( result->total_frames( ) == 0 )
			result = aml_index_reader_ptr( );
	}

	if ( !result )
	{
		// Try V2 Index
		result = aml_index_reader_ptr( new aml_index_reader< awi_parser_v2 >( indexname ) );
		if ( result->total_frames( ) == 0 )
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

		void job_request( )
		{
			index_->read( );
			set_should_reschedule( !get_should_terminate_job() && !finished( ) );
		}

		const boost::posix_time::milliseconds job_delay( ) const
		{
			return boost::posix_time::milliseconds( 100 );
		}

	private:
		pl::wstring url_;
		aml_index_reader_ptr index_;
};

#define RETRIES 60

class unindexed_job_type : public indexer_job 
{
	public:
		unindexed_job_type( const pl::wstring &url )
			: url_( url )
			, size_( 0 )
			, finished_( false )
			, retries_( RETRIES )
		{
			if ( url_.find( L":cache:" ) != pl::wstring::npos )
				url_ = url.substr( 0, url.find( L"cache:" ) ) + url.substr( url.find( L"cache:" ) + 6 );
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
			return finished_;
		}

		void job_request( )
		{
			check_size( );
			set_should_reschedule( !get_should_terminate_job() && !finished( ) );
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
			if ( ffurl_open( &context, temp.c_str( ), AVIO_FLAG_READ, 0, 0 ) >= 0 )
			{
				// Check the current file size
				boost::int64_t bytes = ffurl_seek( context, 0, SEEK_END );

				// Clean up the temporary sizing context
				ffurl_close( context );

				// If it's larger than before, then reopen, otherwise reduce resize count
				{
					boost::recursive_mutex::scoped_lock lock( mutex_ );
					if ( bytes > size_ )
					{
						size_ = bytes;
						retries_ = RETRIES;
					}
					else
					{
						finished_ = -- retries_ == 0;
					}
				}
			}
		}

	private:
		mutable boost::recursive_mutex mutex_;
		pl::wstring url_;
		boost::int64_t size_;
		bool finished_;
		int retries_;
};

static const pl::pcos::key key_offset_ = pl::pcos::key::from_string( "offset" );
static const pl::pcos::key key_file_size_ = pl::pcos::key::from_string( "file_size" );
static const pl::pcos::key key_frame_size_ = pl::pcos::key::from_string( "frame_size" );

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
					int length = 0;

					if ( last_frame_->get_stream( )->properties( ).get_property_with_key( key_frame_size_ ).valid( ) )
						length = last_frame_->get_stream( )->properties( ).get_property_with_key( key_frame_size_ ).value< int >( );

					index_->enroll( last_frame_->get_position( ), last_frame_->get_stream( )->properties( ).get_property_with_key( key_offset_ ).value< boost::int64_t >( ), length );
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
			return index_->finished( );
		}

		void job_request( )
		{
			analyse_gop( );
			set_should_reschedule( !get_should_terminate_job() && !finished( ) );
		}

		const boost::posix_time::milliseconds job_delay( ) const
		{
			return boost::posix_time::milliseconds( 100 );
		}


	private:
		void analyse_gop( )
		{
			boost::posix_time::ptime end_time = boost::posix_time::microsec_clock::local_time( ) + boost::posix_time::milliseconds( 200 );
			int registered = 0;
			pl::pcos::property complete = input_->properties( ).get_property_with_key( key_complete_ );

			input_->sync( );

			while ( input_ && start_ < input_->get_frames( ) )
			{
				registered ++;
				last_frame_ = input_->fetch( start_ ++ );

				int length = 0;
				if ( last_frame_ && last_frame_->get_stream( ) && last_frame_->get_stream( )->properties( ).get_property_with_key( key_frame_size_ ).valid( ) )
					length = last_frame_->get_stream( )->properties( ).get_property_with_key( key_frame_size_ ).value< int >( );

				if ( last_frame_->get_stream( )->position( ) == last_frame_->get_stream( )->key( ) )
				{
					index_->enroll( last_frame_->get_position( ), last_frame_->get_stream( )->properties( ).get_property_with_key( key_offset_ ).value< boost::int64_t >( ), length );
					if (  boost::posix_time::microsec_clock::local_time( ) > end_time && registered > 100 && !( complete.valid( ) && complete.value< int >( ) == 1 ) )
						break;
				}
				else
				{
					index_->detail( last_frame_->get_position( ), last_frame_->get_stream( )->properties( ).get_property_with_key( key_offset_ ).value< boost::int64_t >( ), length );
				}
			}

			if ( complete.valid( ) && complete.value< int >( ) == 1 )
			{
				input_->sync( );
				if ( last_frame_ && start_ >= input_->get_frames( ) )
                	index_->close( input_->get_frames( ), input_->properties( ).get_property_with_key( key_file_size_ ).value< boost::int64_t >( ) );				

				//Release the input, since we don't need it any more
				input_.reset( );
			}
		}

		pl::wstring url_;
		awi_generator_v3_ptr index_;
		input_type_ptr input_;
		int start_;
		frame_type_ptr last_frame_;
};

static indexer_job_ptr indexer_job_factory( const pl::wstring &url, boost::uint16_t v4_index_entry_type )
{
	if ( url.find( L"index:" ) != 0 )
	{
		aml_index_reader_ptr index = create( pl::to_string( url ), v4_index_entry_type );
		if ( index )
			return indexer_job_ptr( new indexed_job_type( url, index ) );
		else if ( url.find( L".awi" ) == pl::wstring::npos )
			return indexer_job_ptr( new unindexed_job_type( url ) );
		else
			return indexer_job_ptr( );
	}
	else
	{
		return indexer_job_ptr( new generating_job_type( url ) );
	}
}

class indexer;
typedef boost::shared_ptr< indexer > indexer_ptr;

// The indexer singleton
class indexer
{
	public:
		/// Provides the singleton instance of this object
		static indexer_ptr instance( )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			if ( !instance_ )
				instance_ = indexer_ptr( new indexer( ) );
			return instance_;
		}

		static void shutdown( )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			if ( instance_ )
			{
				instance_->index_read_worker_.stop( boost::posix_time::seconds( 5 ) );
				instance_ = indexer_ptr( );
			}
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
		indexer_item_ptr request( const pl::wstring &url, boost::uint16_t v4_index_entry_type = 0 )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			ARLOG_DEBUG5( "Index request for url: %1%" )( url );

			//Decorate the lookup key with the type of index entry, so that we don't
			//reuse the same index job for a different entry type
			std::wstringstream map_key_str;
			map_key_str << url;
			if( v4_index_entry_type != 0)
			{
				map_key_str << L":" << v4_index_entry_type;
			}
			pl::wstring map_key = map_key_str.str();

			map_type::iterator existing = map_.find( map_key );

			if ( existing == map_.end( ) )
			{
				indexer_job_ptr job = indexer_job_factory( url, v4_index_entry_type );
				if ( !job ) return indexer_item_ptr( );

				bool has_frames_or_size = ( ( job->index( ) && job->index( )->total_frames( ) > 0 ) || ( !job->index( ) && job->size( ) > 0 ) );
				if ( !job->finished( ) && has_frames_or_size )
				{
					index_read_worker_.add_reoccurring_job( job, job->job_delay( ) );
				}
				if ( has_frames_or_size )
				{
					existing = map_.insert( std::make_pair( map_key, std::make_pair( job, 1 ) ) ).first;
				}
				else
				{
					ARLOG_ERR("Index job creation failed for url %1%")( url );
					return indexer_item_ptr( );
				}
			}
			else
			{
				++existing->second.second;
			}

			return existing->second.first;
		}

		void cancel( indexer_item_ptr item )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );

			map_type::iterator it;
			for( it = map_.begin(); it != map_.end(); ++it )
			{
				if( it->second.first == item )
				{
					break;
				}
			}

			if( it != map_.end() )
			{
				boost::int32_t &ref_count = it->second.second;
				--ref_count;
				if( ref_count == 0 )
				{
					if( !it->second.first->finished() )
					{
						if( !index_read_worker_.cancel_and_remove_job( it->second.first, boost::posix_time::milliseconds( 5000 ) ) )
						{
							ARLOG_ERR( "Wait timed out when attempting to release indexer item job" );
						}
					}

					map_.erase( it );
				}
			}
			else
			{
				ARLOG_DEBUG( "Attempting to cancel non-existent indexer item job. Already cancelled?" );
			}
		}

	private:
		/// Constructor for the indexer collection
		indexer( )
		: map_( )
		, index_read_worker_( _CT("Indexer") )
		{
			index_read_worker_.start( );
		}

		static boost::recursive_mutex mutex_;
		static indexer_ptr instance_;

		typedef std::map< pl::wstring, std::pair< indexer_job_ptr, boost::int32_t > > map_type;
		map_type map_;
		opencorelib::worker index_read_worker_;
};

indexer_ptr indexer::instance_;
boost::recursive_mutex indexer::mutex_;

void ML_DECLSPEC indexer_init( )
{
	indexer::instance( )->init( );
}

indexer_item_ptr ML_DECLSPEC indexer_request( const openpluginlib::wstring &url, boost::uint16_t v4_index_entry_type )
{
	return indexer::instance( )->request( url, v4_index_entry_type );
}

void ML_DECLSPEC indexer_cancel_request( const indexer_item_ptr &item )
{
	indexer::instance( )->cancel( item );
}

void ML_DECLSPEC indexer_shutdown( )
{
	indexer::shutdown( );
}

} } }

