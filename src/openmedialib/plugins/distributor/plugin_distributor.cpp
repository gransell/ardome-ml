// distributor - A distributor plugin to ml.
//
// Copyright (C) 2010 Vizrt
// Released under the LGPL.

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/stack.hpp>
#include <openpluginlib/pl/pcos/observer.hpp>
#include <opencorelib/cl/thread_pool.hpp>
#include <opencorelib/cl/function_job.hpp>
#include <opencorelib/cl/enforce_defines.hpp>
#include <opencorelib/cl/log_defines.hpp>
#include <opencorelib/cl/lru.hpp>

namespace pl = olib::openpluginlib;
namespace ml = olib::openmedialib::ml;
namespace il = olib::openimagelib::il;
namespace pcos = olib::openpluginlib::pcos;
namespace cl = olib::opencorelib;

namespace olib { namespace openmedialib { namespace ml { namespace distributor {

class ML_PLUGIN_DECLSPEC filter_lock : public filter_simple
{
	public:
		virtual bool requires_image( ) const { return false; }

		virtual const pl::wstring get_uri( ) const { return pl::wstring( L"lock" ); }

		virtual const size_t slot_count( ) const { return 1; }

		virtual void seek( const int position, const bool relative = false )
		{
			boost::recursive_mutex::scoped_lock lck( mutex_ );
			if ( position_.get( ) == 0 ) position_.reset( new int( 0 ) );
			int *current = position_.get( );
			*current = opencorelib::utilities::clamp( relative ? *current + position : position, 0, int( get_frames( ) - 1 ) );
		}

		virtual int get_position( ) const
		{
			boost::recursive_mutex::scoped_lock lck( mutex_ );
			return position_.get( ) ? *( position_.get( ) ) : 0;
		}

	protected:
		void do_fetch( frame_type_ptr &frame )
		{
			boost::recursive_mutex::scoped_lock lck( mutex_ );
			frame = fetch_from_slot( );
		}

	private:
		boost::thread_specific_ptr< int > position_;
		mutable boost::recursive_mutex mutex_;
};

static pl::pcos::key key_audio_reversed_ = pl::pcos::key::from_string( "audio_reversed" );

template < typename T > 
class observer : public pcos::observer
{
	public:
		observer( T *instance, void ( T::*fn )( ) )
		: instance_( instance )
		, fn_( fn )
		{
		}

		virtual void updated( pcos::isubject * )
		{
			( instance_->*fn_ )( ); 
		}

	private:
		T *instance_;
		void ( T::*fn_ )( );
};

class ML_PLUGIN_DECLSPEC filter_distribute : public filter_simple
{
	public:
		filter_distribute( )
		: obs_active_( new observer< filter_distribute >( this, &filter_distribute::update_active ) )
		, prop_threads_( pl::pcos::key::from_string( "threads" ) )
		, prop_queue_( pl::pcos::key::from_string( "queue" ) )
		, prop_active_( pl::pcos::key::from_string( "active" ) )
		, prop_audio_direction_( pl::pcos::key::from_string( "audio_direction" ) )
		, prop_trigger_( pl::pcos::key::from_string( "trigger" ) )
		, pool_( 0 )
		, expected_( 0 )
		, requested_( 0 )
		, direction_( 1 )
		{
			properties( ).append( prop_threads_ = 1 );
			properties( ).append( prop_queue_ = 25 );
			properties( ).append( prop_active_ = 1 );
			properties( ).append( prop_audio_direction_ = 1 );
			properties( ).append( prop_trigger_ = 1 );
			prop_active_.attach( obs_active_ );
		}

		virtual ~filter_distribute( )
		{
			if ( pool_ )
			{
				pool_->clear_jobs( );
				pool_->wait_for_all_jobs_completed( boost::posix_time::seconds( 5 ) );
			}
			delete pool_;
		}

		virtual bool requires_image( ) const { return false; }

		virtual const pl::wstring get_uri( ) const { return pl::wstring( L"distribute" ); }

		virtual const size_t slot_count( ) const { return 1; }

		// Honour active property modifications
		void update_active( )
		{
			boost::posix_time::time_duration timeout = boost::posix_time::seconds( 5 );

			if ( fetch_slot( ) == input_type_ptr( ) )
			{
				// Assignment prior to connection - we don't know if the graph is threadsafe
				// at this point, so do nothing - we'll handle the active state on any post 
				// connection assignments or on the first fetch
			}
			else if ( is_thread_safe( ) )
			{
				if ( prop_active_.value< int >( ) == 1 )
				{
					if ( pool_ )
						clone_graphs( prop_threads_.value< int >( ) );
				}
				else
				{
					if ( pool_ )
					{
						pool_->clear_jobs( );
						pool_->wait_for_all_jobs_completed( timeout );
					}
					lru_.clear( );
					clear_graphs( );
				}
			}
			else
			{
				prop_active_ = 0;
			}
		}

	protected:
		// Obtain the currently requested frame using either the threaded or non-threaded
		// mechanism (dependent on active property and thread safety of the attached graph)
		void do_fetch( frame_type_ptr &frame )
		{
			// Check for thread safety on first frame
			if ( prop_active_.value< int >( ) == 1 && !pool_ )
				update_active( );

			// Route here for threaded or non-threaded fetches
			if ( prop_active_.value< int >( ) == 0 )
				do_non_threaded_fetch( frame );
			else
				do_threaded_fetch( frame );

			// Shallow copy and correct audio direction
			if ( frame )
			{
				frame = frame->shallow( );
				handle_reverse_audio( frame );
			}
		}

	private:
		void do_non_threaded_fetch( frame_type_ptr &frame )
		{
			int position = get_position( );
			int queue = prop_queue_.value< int >( );

			lru_.resize( queue );
			frame = lru_.fetch( position );

			if ( !frame )
			{
				frame = fetch_from_slot( 0 );
				lru_.append( position, frame );
			}
		}

		void do_threaded_fetch( frame_type_ptr &frame )
		{
			boost::posix_time::time_duration timeout = boost::posix_time::seconds( 5 );
			int position = get_position( );
			int threads = prop_threads_.value< int >( );
			int queue = prop_queue_.value< int >( );
			bool paused = position == expected_ - direction_;

			if ( pool_ == 0 )
			{
				clone_graphs( threads );
				lru_.resize( queue );
				pool_ = new cl::thread_pool( threads, timeout );
			}

			frame = lru_.fetch( position );

			if ( expected_ != position && !paused )
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

			expected_ = position + direction_;
		}

		void clone_graphs( int graphs )
		{
			boost::recursive_mutex::scoped_lock lck( mutex_ );
			while( graphs -- )
			{
				stack parser;
				clone( parser, fetch_slot( 0 ), shared_from_this( ) );
				graphs_.push_back( parser.pop( ) );
			}
		}

		void clear_graphs( )
		{
			boost::recursive_mutex::scoped_lock lck( mutex_ );
			graphs_.clear( );
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
				frame_type_ptr frame = input->fetch( position );

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

		void handle_reverse_audio( frame_type_ptr result )
		{
			if ( result && prop_audio_direction_.value< int >( ) )
			{
				if ( result->properties( ).get_property_with_key( key_audio_reversed_ ).valid( ) )
				{
					pl::pcos::property audio_reversed = result->properties( ).get_property_with_key( key_audio_reversed_ );
					if ( ( audio_reversed.value< int >( ) && direction_ >= 0 ) || ( !audio_reversed.value< int >( ) && direction_ < 0 ) )
					{
						result->set_audio( audio::reverse( result->get_audio( ) ) );
						audio_reversed = !audio_reversed.value< int >( );
					}
				}
				else
				{
					pl::pcos::property audio_reversed( key_audio_reversed_ );
					if ( direction_ < 0 )
					{
						result->set_audio( audio::reverse( result->get_audio( ) ) );
					}
					result->properties( ).append( audio_reversed = direction_ < 0 ? 1 : 0 );
				}
			}
		}

	private:
		boost::shared_ptr< pl::pcos::observer > obs_active_;
		pl::pcos::property prop_threads_;
		pl::pcos::property prop_queue_;
		pl::pcos::property prop_active_;
		pl::pcos::property prop_audio_direction_;
		pl::pcos::property prop_trigger_;
		std::vector< input_type_ptr > graphs_;
		cl::lru< int, frame_type_ptr > lru_;
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
	virtual filter_type_ptr filter( const pl::wstring &spec )
	{
		if ( spec == L"lock" )
			return filter_type_ptr( new filter_lock( ) );
		if ( spec == L"distribute" )
			return filter_type_ptr( new filter_distribute( ) );
		return filter_type_ptr( );
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
