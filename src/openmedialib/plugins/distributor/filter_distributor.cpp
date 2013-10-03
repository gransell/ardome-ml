// Copyright (C) 2010-2013 Vizrt
// Released under the LGPL.
//
// #filter:distributor
//
// A replacement for the filter:threader.
//

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/stack.hpp>
#include <openmedialib/ml/keys.hpp>
#include <openpluginlib/pl/pcos/observer.hpp>
#include <opencorelib/cl/thread_pool.hpp>
#include <opencorelib/cl/function_job.hpp>
#include <opencorelib/cl/enforce_defines.hpp>
#include <opencorelib/cl/log_defines.hpp>
#include <opencorelib/cl/thread_cache.hpp>
#include <opencorelib/cl/thread_monitor.hpp>

namespace pl = olib::openpluginlib;
namespace ml = olib::openmedialib::ml;
namespace il = olib::openimagelib::il;
namespace cl = olib::opencorelib;

namespace olib { namespace openmedialib { namespace ml { namespace distributor {

template < typename T > 
class observer : public pl::pcos::observer
{
	public:
		observer( T *instance, void ( T::*fn )( ) )
		: instance_( instance )
		, fn_( fn )
		{
		}

		virtual void updated( pl::pcos::isubject * )
		{
			( instance_->*fn_ )( ); 
		}

	private:
		T *instance_;
		void ( T::*fn_ )( );
};

class graphs
{
	public:
		// Wrapper for the cloned graphs - ensures one is obtained and released when required
		class item
		{
			public:
				item( graphs *collection, int position )
				: collection_( collection )
				, position_( position )
				, input_( collection_->fetch_graph( ) )
				{ }

				~item( )
				{ collection_->return_graph( input_, position_ ); }

				frame_type_ptr fetch( )
				{ return input_->fetch( position_ ); }

			private:
				graphs *collection_;
				int position_;
				input_type_ptr input_;
		};

		// Create a clone of the graph for each thread
		void clone_graphs( input_type_ptr input, int graphs )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			if ( graphs < 0 ) graphs = boost::thread::hardware_concurrency( );
			int total = graphs;
			while( graphs -- )
			{
				stack parser;
				clone( total, parser, input->fetch_slot( ), input );
				graphs_.push_back( parser.pop( ) );
			}
		}

		// Synchronise all the graphs in the collection - care should be taken to
		// ensure they all sync with the same frame count
		void sync_graphs( )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			for ( std::vector< input_type_ptr >::iterator iter = graphs_.begin( ); iter != graphs_.end( ); ++iter )
				( *iter )->sync( );
		}

		// Clear all the graphs
		void clear_graphs( )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			graphs_.clear( );
		}

		// Clear all scheduled frame requests
		void clear_schedule( )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			scheduled_.clear( );
		}

		// Mark a frame as scheduled
		void schedule( int position )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			scheduled_[ position ] = position;
		}

		// Determine if a frame is scheduled
		bool scheduled( int position )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			return scheduled_.find( position ) != scheduled_.end( );
		}

	private:
		// Clone a graph taking into account special case filters (such as nested threaders which
		// are responsible for their own subgraphs, decode filters which provide deferred evaluation,
		// inputs which are considered as terminating points and locks which force synchronisation).
		void clone( int graphs, stack &parser, input_type_ptr graph, input_type_ptr parent, size_t parent_slot = 0 )
		{
			ARENFORCE_MSG( graph, "Malformed graph - null input detected" );

			if ( graph->slot_count( ) == 0 || 
				 graph->get_uri( ) == L"decode" || 
				 graph->get_uri( ) == L"distributor" || 
				 graph->get_uri( ) == L"threader" || 
				 graph->get_uri( ) == L"voodoo" || 
				 ( graph->get_uri( ) == L"store" && graphs != 1 ) || 
				 graph->get_uri( ) == L"tee" )
			{
				filter_type_ptr lock = create_filter( L"lock" );
				pl::pcos::assign< std::wstring >( lock->properties( ), ml::keys::serialise_as, L"" );
				lock->connect( graph );
				parent->connect( lock, parent_slot );
				parser.push( lock );
			}
			else if ( graph->get_uri( ) == L"lock" )
			{
				parser.push( graph );
			}
			else
			{
				for ( size_t i = 0; i < graph->slot_count( ); i ++ )
					clone( graphs, parser, graph->fetch_slot( i ), graph, i );
				parser.copy( graph );
			}
		}

		// Obtain a clone to do the real work
		input_type_ptr fetch_graph( )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			input_type_ptr input = graphs_.back( );
			graphs_.pop_back( );
			return input;
		}

		// Return a clone to the pool for reuse
		void return_graph( input_type_ptr input, int position )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			graphs_.push_back( input );
			scheduled_.erase( scheduled_.find( position ) );
		}

		boost::recursive_mutex mutex_;
		std::vector< input_type_ptr > graphs_;
		std::map< int, int > scheduled_;
};

static pl::pcos::key key_audio_reversed_ = pl::pcos::key::from_string( "audio_reversed" );
static pl::pcos::key key_active_ = pl::pcos::key::from_string( "active" );
static pl::pcos::key key_sync_ = pl::pcos::key::from_string( "sync" );

class ML_PLUGIN_DECLSPEC filter_distributor : public filter_simple
{
	public:
		filter_distributor( )
		: obs_active_( new observer< filter_distributor >( this, &filter_distributor::update_active ) )
		, last_used_( boost::get_system_time( ) )
		, prop_threads_( pl::pcos::key::from_string( "threads" ) )
		, prop_queue_( pl::pcos::key::from_string( "queue" ) )
		, prop_active_( pl::pcos::key::from_string( "active" ) )
		, prop_audio_direction_( pl::pcos::key::from_string( "audio_direction" ) )
		, prop_trigger_( pl::pcos::key::from_string( "trigger" ) )
		, prop_timeout_( pl::pcos::key::from_string( "timeout" ) )
		, monitor_( cl::function_job_ptr( new cl::function_job( boost::bind( &filter_distributor::monitor, this, 5 ) ) ) )
		, lru_key_( lru_frame_cache::allocate( ) )
		, initialised_( false )
		, runnable_( true )
		, expected_( -1 )
		, direction_( 1 )
		, previous_( 0 )
		{
			properties( ).append( prop_threads_ = 1 );
			properties( ).append( prop_queue_ = 25 );
			properties( ).append( prop_active_ = 1 );
			properties( ).append( prop_audio_direction_ = 1 );
			properties( ).append( prop_trigger_ = 1 );
			properties( ).append( prop_timeout_ = 5000 );
			prop_active_.attach( obs_active_ );
		}

		virtual ~filter_distributor( )
		{
			release_pool( );
		}

		virtual bool requires_image( ) const { return false; }

		virtual const std::wstring get_uri( ) const { return std::wstring( L"distributor" ); }

		// We can only connect 1 input to this filter
		virtual const size_t slot_count( ) const { return 1; }

		void sync( )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ ); 
			clean_pool( );
			set_sync( 1 );
			filter_simple::sync( );
			set_sync( 0 );
			graphs_.sync_graphs( );
		}

		// Honour active property modifications
		void update_active( )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );

			if ( fetch_slot( ) == input_type_ptr( ) )
			{
				// Assignment prior to connection - we don't know if the graph is threadsafe
				// at this point, so will defer the decision until any post connection 
				// assignments or the first frame (depending on what happens first)
				initialised_ = false;
				runnable_ = true;
			}
			else if ( prop_threads_.value< int >( ) != 0 && is_thread_safe( ) )
			{
				initialised_ = false;
				runnable_ = true;
				release_pool( );
			}
			else
			{
				initialised_ = true;
				runnable_ = false;
			}

			// Locate any upstream threaders and assign their active state as required
			set_active( prop_active_.value< int >( ) );
		}

	protected:
		// Obtain the currently requested frame using either the threaded or non-threaded
		// mechanism (dependent on active property and thread safety of the attached graph)
		void do_fetch( frame_type_ptr &frame )
		{
			// Route here for threaded or non-threaded fetches
			if ( threaded( ) )
				do_threaded_fetch( frame );
			else
				do_non_threaded_fetch( frame );

			// Shallow copy and correct audio direction
			if ( frame )
			{
				frame = frame->shallow( );
				handle_reverse_audio( frame );
			}
		}

	private:
		// Determine if we're running in threaded or non-threaded mode here
		bool threaded( )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );

			// Track the last use of this filter
			last_used_ = boost::get_system_time( );

			// Check for thread safety on first frame
			if ( prop_active_.value< int >( ) == 1 && !initialised_ && runnable_ )
			{
				// Check for thread safety here
				update_active( );

				// If we're runnable clone our graphs now to complete initialisation
				if ( runnable_ )
					graphs_.clone_graphs( shared_from_this( ), prop_threads_.value< int >( ) );

				initialised_ = true;
			}

			return initialised_ && runnable_;
		}

		// Fetch and cache the requested frame
		void do_non_threaded_fetch( frame_type_ptr &frame )
		{
			int position = get_position( );
			int queue = prop_queue_.value< int >( );

			lru_frame_ptr lru = lru_frame_cache::fetch( lru_key_ );

			lru->resize( queue );
			frame = lru->fetch( position );

			if ( !frame )
			{
				frame = trigger( fetch_from_slot( 0 ) );
				lru->append( position, frame );
			}
		}

		// Fetch and schedule predicted frame requests
		void do_threaded_fetch( frame_type_ptr &frame )
		{
			boost::posix_time::time_duration timeout = boost::posix_time::milliseconds( prop_timeout_.value< int >( ) );
			int position = get_position( );
			int queue = prop_queue_.value< int >( );

			// Obtain the frame lru associated to this threader
			lru_frame_ptr lru = lru_frame_cache::fetch( lru_key_ );

			// Ensure the lru object is sized correctly
			lru->resize( queue );

			// If the position has changed from what we expect and the position isn't pending, clear
			// all jobs and schedule for this position - otherwise, schedule additional jobs
			if ( expected_ != position && !pending( position ) )
			{
				clean_pool( );
				add_job( position );
				direction_ = position == previous_ ? 1 : position - previous_;
			}
			else if ( expected_ == position )
			{
				int min_extremity = std::max< int >( position - std::abs( direction_ ) * ( queue / 2 ), 0 );
				int max_extremity = std::min< int >( position + std::abs( direction_ ) * ( queue / 2 ), get_frames( ) );
				int requested = position;

				while( requested >= min_extremity && requested < max_extremity )
				{
					add_job( requested );
					requested += direction_;
				}
			}
			else
			{
				direction_ = position == previous_ ? 1 : position - previous_;
			}

			// Wait for the position to be become available
			frame = lru->wait( position, timeout );

			// Handle the state for the next frame
			previous_ = position;
			expected_ = position + direction_;
		}

		// Walk the graph to set the active property on the lowest threader(s)
		void set_active( int value )
		{
			set_property( fetch_slot( ), L"distributor", key_active_, value );
		}

		// Walk the graph to set the sync property on the lowest sync filter(s)
		void set_sync( int value )
		{
			set_property( fetch_slot( ), L"sync", key_sync_, value );
		}

		// Locate the first upstream filters with a given name and assign value to the property specified by the key
		void set_property( input_type_ptr input, const std::wstring &name, const pl::pcos::key &key, int value )
		{
			if ( input && input->get_uri( ) == name )
			{
				input->properties( ).get_property_with_key( key ) = value;
			}
			else if ( input )
			{
				for ( size_t i = 0; i < input->slot_count( ); i ++ )
					set_property( input->fetch_slot( i ), name, key, value );
			}
		}

		// Schedule a job for a frame
		void add_job( int position )
		{
			lru_frame_ptr lru = lru_frame_cache::fetch( lru_key_ );
			if ( !pending( position ) )
			{
				graphs_.schedule( position );
				cl::function_job_ptr fjob( new cl::function_job( boost::bind( &filter_distributor::decode_job, this, position ) ) );
				acquire_pool( )->add_job( fjob );
			}
		}

		// Carry out the job
		void decode_job( int position )
		{
			graphs::item input( &graphs_, position );
			frame_type_ptr frame = trigger( input.fetch( ) );
			lru_frame_ptr lru = lru_frame_cache::fetch( lru_key_ );
			lru->append( position, frame );
		}

		// Process the frame according to the trigger
		frame_type_ptr trigger( frame_type_ptr frame )
		{
			if ( frame )
			{
				int trigger = prop_trigger_.value< int >( );
				if ( trigger & 1 ) frame->get_image( );
				if ( trigger & 2 ) frame->get_stream( );
				if ( trigger & 4 ) frame->get_audio( );
			}
			return frame;
		}

		// Obtain a thread pool for the number of threads associated to this filter
		cl::thread_pool_ptr acquire_pool( )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			if ( !pool_ )
			{
				int threads = prop_threads_.value< int >( );
				if ( threads < 0 ) threads = boost::thread::hardware_concurrency( );
				pool_ = cl::thread_cache::acquire( threads );
				cl::thread_monitor::enroll( monitor_ );
			}
			last_used_ = boost::get_system_time( );
			return pool_;
		}

		// Clean up any jobs associated to an allocated pool
		void clean_pool( )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			if ( pool_ )
			{
				boost::posix_time::time_duration timeout = boost::posix_time::seconds( 5 );
				pool_->clear_jobs( );
				pool_->wait_for_all_jobs_completed( timeout );
			}
			graphs_.clear_schedule( );
		}

		// Clean up and release an allocated pool (if any)
		void release_pool( )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			cl::thread_monitor::withdraw( monitor_ );
			clean_pool( );
			cl::thread_cache::release( pool_ );
			pool_ = cl::thread_pool_ptr( );
			lru_frame_cache::deallocate( lru_key_ );
			graphs_.clear_graphs( );
			initialised_ = false;
		}

		// Invoked via the thread_monitor - checks if we've not been used for a while and releases 
		// resources as necessary
		void monitor( int seconds )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			boost::system_time now = boost::get_system_time( );
			boost::posix_time::time_duration diff = now - last_used_;
			if ( diff > boost::posix_time::seconds( seconds ) ) 
				release_pool( );
		}

		// Determine if a frame is scheduled or available
		bool pending( int position )
		{
			lru_frame_ptr lru = lru_frame_cache::fetch( lru_key_ );
			return graphs_.scheduled( position ) || lru->fetch( position ) != frame_type_ptr( );
		}

		// Correct the audio sample order
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
						audio_reversed = audio_reversed.value< int >( ) ? 0 : 1;
					}
				}
				else
				{
					pl::pcos::property audio_reversed( key_audio_reversed_ );
					if ( direction_ < 0 )
						result->set_audio( audio::reverse( result->get_audio( ) ) );
					result->properties( ).append( audio_reversed = direction_ < 0 ? 1 : 0 );
				}
			}
		}

	private:
		boost::recursive_mutex mutex_;
		boost::shared_ptr< pl::pcos::observer > obs_active_;
		boost::system_time last_used_;
		pl::pcos::property prop_threads_;
		pl::pcos::property prop_queue_;
		pl::pcos::property prop_active_;
		pl::pcos::property prop_audio_direction_;
		pl::pcos::property prop_trigger_;
		pl::pcos::property prop_timeout_;
		cl::function_job_ptr monitor_;
		cl::lru_key lru_key_;
		cl::thread_pool_ptr pool_;
		graphs graphs_;
		bool initialised_;
		bool runnable_;
		int expected_;
		int direction_;
		int previous_;
};

filter_type_ptr create_distributor()
{
	return filter_type_ptr( new filter_distributor() );
}

} } } }
