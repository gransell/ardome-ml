// distributor - A distributor plugin to ml.
//
// Copyright (C) 2010 Vizrt
// Released under the LGPL.

#include <openmedialib/ml/analyse_mpeg2.hpp>
#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/packet.hpp>
#include <openpluginlib/pl/pcos/observer.hpp>
#include <opencorelib/cl/thread_pool.hpp>
#include <opencorelib/cl/function_job.hpp>
#include <opencorelib/cl/profile.hpp>
#include <opencorelib/cl/enforce_defines.hpp>
#include <opencorelib/cl/log_defines.hpp>
#include <opencorelib/cl/uuid_16b.hpp>
#include <opencorelib/cl/str_util.hpp>
#include <openmedialib/ml/filter_encode.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <iostream>
#include <cstdlib>
#include <vector>
#include <string>

namespace pl = olib::openpluginlib;
namespace ml = olib::openmedialib::ml;
namespace il = olib::openimagelib::il;
namespace pcos = olib::openpluginlib::pcos;
namespace cl = olib::opencorelib;

namespace olib { namespace openmedialib { namespace ml { namespace distributor {

class ML_PLUGIN_DECLSPEC filter_lock : public filter_simple
{
	public:
		filter_lock( )
		: filter_simple( )
		{
		}

		virtual ~filter_lock( )
		{
		}

		virtual bool requires_image( ) const { return false; }

		virtual const pl::wstring get_uri( ) const { return pl::wstring( L"lock" ); }

		virtual const size_t slot_count( ) const { return 1; }

		virtual void seek( const int position, const bool relative = false )
		{
			int *current = position_ptr( );
			*current = opencorelib::utilities::clamp( relative ? *current + position : position, 0, int( get_frames( ) - 1 ) );
		}

		virtual int get_position( ) const
		{
			return position_.get( ) ? *( position_.get( ) ) : 0;
		}

	protected:

		int *position_ptr( )
		{
			if ( position_.get( ) == 0 ) position_.reset( new int( 0 ) );
			return position_.get( );
		}

		void do_fetch( frame_type_ptr &frame )
		{
			boost::recursive_mutex::scoped_lock lck( mutex_ );
			frame = fetch_from_slot( );
		}

	private:
		boost::thread_specific_ptr< int > position_;
		boost::recursive_mutex mutex_;
};

template< typename Tkey, typename Tval >
class lru
{
	public:
		typedef Tkey key_type;
		typedef Tval val_type;
		typedef std::map< key_type, val_type > map;
		typedef typename map::iterator iterator;
		typedef std::list< key_type > list;

		lru( )
		: size_( 50 )
		{
		}

		void resize( size_t size )
		{
			boost::recursive_mutex::scoped_lock lck( mutex_ );
			while( queue_.size( ) > size )
			{
				queue_.erase( queue_.find( *( lru_.begin( ) ) ) );
				lru_.pop_front( );
			}
			size_ = size;
		}

		void append( key_type index, val_type frame )
		{
			boost::recursive_mutex::scoped_lock lck( mutex_ );
			queue_[ index ] = frame;
			lru_.push_back( index );
			resize( size_ );
			cond_.notify_all( );
		}

		val_type fetch( key_type index )
		{
			boost::recursive_mutex::scoped_lock lck( mutex_ );
			val_type result;
			iterator iter = queue_.find( index );
			if ( iter != queue_.end( ) )
			{
				result = iter->second;
				lru_.remove( index );
				lru_.push_back( index );
			}
			return result;
		}

		val_type wait( key_type index, boost::posix_time::time_duration time )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			val_type result;
			while( !( result = fetch( index ) ) )
				if ( !cond_.timed_wait( lock, time ) )
					break;
			return result;
		}

	private:
		boost::recursive_mutex mutex_;
		boost::condition_variable_any cond_;
		map queue_;
		list lru_;
		size_t size_;
};

class stack
{
	public:
		stack( )
		: stack_( create_input( L"aml_stack:" ) )
		{
		}

		void push( input_type_ptr input )
		{
			stack_->connect( input );
			push( L"recover" );
		}

		void push( pl::wstring command )
		{
			stack_->property( "parse" ) = command;
		}

		void copy( input_type_ptr &input )
		{
			filter_type_ptr aml = create_filter( L"aml" );
			aml->property( "limit" ) = 1;
			aml->property( "filename" ) = pl::wstring( L"@" );
			aml->connect( input );
			pl::wstring output = pl::to_wstring( aml->property( "stdout" ).value< std::string >( ) );
			push( output );
		}

		input_type_ptr pop( )
		{
			push( L"." );
			return stack_->fetch_slot( 0 );
		}

	private:
		input_type_ptr stack_;
};

class ML_PLUGIN_DECLSPEC filter_distribute : public filter_simple
{
	public:
		filter_distribute( )
		: filter_simple( )
		, prop_threads_( pl::pcos::key::from_string( "threads" ) )
		, prop_queue_( pl::pcos::key::from_string( "queue" ) )
		, prop_active_( pl::pcos::key::from_string( "active" ) )
		, prop_trigger_( pl::pcos::key::from_string( "trigger" ) )
		, pool_( 0 )
		, expected_( 0 )
		, requested_( 0 )
		, direction_( 1 )
		{
			properties( ).append( prop_threads_ = 4 );
			properties( ).append( prop_queue_ = 50 );
			properties( ).append( prop_active_ = 1 );
			properties( ).append( prop_trigger_ = 1 );
		}

		virtual ~filter_distribute( )
		{
			if ( pool_ )
				pool_->terminate_all_threads( boost::posix_time::seconds( 5 ) );
			delete pool_;
		}

		virtual bool requires_image( ) const { return false; }

		virtual const pl::wstring get_uri( ) const { return pl::wstring( L"distribute" ); }

		virtual const size_t slot_count( ) const { return 1; }

	protected:

		void do_fetch( frame_type_ptr &frame )
		{
			boost::posix_time::time_duration timeout = boost::posix_time::seconds( 5 );
			int position = get_position( );
			int threads = prop_threads_.value< int >( );
			int queue = prop_queue_.value< int >( );

			if ( pool_ == 0 )
			{
				clone_graphs( threads );
				lru_.resize( queue );
				pool_ = new cl::thread_pool( threads, timeout );
			}

			frame = lru_.fetch( position );

			if ( expected_ != position && position != expected_ - direction_ )
			{
				pool_->clear_jobs( );
				pool_->wait_for_all_jobs_completed( timeout );
				requested_ = position;
				direction_ = position >= expected_ ? 1 : -1;
			}

			int min_extremity = std::max< int >( position - queue / 2, 0 );
			int max_extremity = std::min< int >( position + queue / 2, get_frames( ) );

			while( requested_ >= min_extremity && requested_ < max_extremity )
			{
				add_job( requested_ );
				requested_ += direction_;
			}

			if ( !frame )
				frame = lru_.wait( position, timeout );

			if ( frame )
				frame = frame->shallow( );

			expected_ = position + direction_;
		}

	private:
		void clone_graphs( int graphs )
		{
			while( graphs -- )
			{
				stack parser;
				clone( parser, fetch_slot( 0 ), shared_from_this( ) );
				graphs_.push_back( parser.pop( ) );
			}
		}

		void clone( stack &parser, input_type_ptr graph, input_type_ptr parent )
		{
			ARENFORCE_MSG( graph, "Malformed graph - null input detected" );

			if ( graph->slot_count( ) == 0 || graph->get_uri( ) == L"decode" || graph->get_uri( ) == L"distribute" )
			{
				filter_type_ptr lock = create_filter( L"lock" );
				lock->connect( graph );
				parent->connect( lock );
				parser.push( lock );
			}
			else if ( graph->get_uri( ) == L"lock" )
			{
				parser.push( graph );
			}
			else
			{
				for ( size_t i = 0; i < graph->slot_count( ); i ++ )
					clone( parser, graph->fetch_slot( i ), graph );
				parser.copy( graph );
			}
		}

		void add_job( int position )
		{
			if ( !lru_.fetch( position ) )
			{
				opencorelib::function_job_ptr fjob( new opencorelib::function_job( boost::bind( &filter_distribute::decode_job, this, position ) ) );
				pool_->add_job( fjob );
			}
		}

		void decode_job( int position )
		{
			input_type_ptr input = fetch_graph( );

			try
			{
				ml::frame_type_ptr frame = input->fetch( position );

				switch( prop_trigger_.value< int >( ) )
				{
					case 1:
						frame->get_image( );
						break;
					case 2:
						frame->get_stream( );
						break;
				}

				lru_.append( position, frame );
			}
			catch( ... )
			{
				return_graph( input );
				throw;
			}

			return_graph( input );
		}

		input_type_ptr fetch_graph( )
		{
			boost::recursive_mutex::scoped_lock lck( mutex_ );
			input_type_ptr input = graphs_.back( );
			graphs_.pop_back( );
			return input;
		}

		void return_graph( input_type_ptr input )
		{
			boost::recursive_mutex::scoped_lock lck( mutex_ );
			graphs_.push_back( input );
		}

	private:
		pl::pcos::property prop_threads_;
		pl::pcos::property prop_queue_;
		pl::pcos::property prop_active_;
		pl::pcos::property prop_trigger_;
		std::vector< input_type_ptr > graphs_;
		lru< int, frame_type_ptr > lru_;
		boost::recursive_mutex mutex_;
		cl::thread_pool *pool_;
		int expected_;
		int requested_;
		int direction_;
};

//
// Plugin object
//

class ML_PLUGIN_DECLSPEC plugin : public openmedialib_plugin
{
public:
	virtual input_type_ptr input( const pl::wstring & )
	{
		return input_type_ptr( );
	}

	virtual store_type_ptr store( const pl::wstring &, const frame_type_ptr & )
	{
		return store_type_ptr( );
	}

	virtual filter_type_ptr filter( const pl::wstring &spec )
	{
		if ( spec == L"lock" )
			return filter_type_ptr( new filter_lock( ) );
		if ( spec == L"distribute" )
			return filter_type_ptr( new filter_distribute( ) );
		return ml::filter_type_ptr( );
	}
};

} } } }

//
// Access methods for openpluginlib
//

extern "C"
{
	ML_PLUGIN_DECLSPEC bool openplugin_init( void )
	{
		return true;
	}

	ML_PLUGIN_DECLSPEC bool openplugin_uninit( void )
	{
		return true;
	}
	
	ML_PLUGIN_DECLSPEC bool openplugin_create_plugin( const char*, pl::openplugin** plug )
	{
		*plug = new ml::distributor::plugin;
		return true;
	}
	
	ML_PLUGIN_DECLSPEC void openplugin_destroy_plugin( pl::openplugin* plug )
	{ 
		delete static_cast< ml::distributor::plugin * >( plug ); 
	}
}
