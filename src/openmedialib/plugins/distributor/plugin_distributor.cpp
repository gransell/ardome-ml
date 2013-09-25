// distributor - A distributor plugin to ml.
//
// Copyright (C) 2010 Vizrt
// Released under the LGPL.
//
// #input:nudger:
//
// This provides an upgrade to the original input:pusher: - it provides a 
// threadsafe implementation which allows pushes and fetches to be carried out
// on two different threads.
//
// #filter:lock
//
// This filter is used to protect non-threadsafe inputs so that they can be 
// safely accessed by multiple concurrent threads. See filter:distributor for
// more details.
//
// #filter:voodoo
//
// This is a variant on the fork filter which allows video and audio filters to
// be carried out on their own sub graphs.
//
// #filter:fork
//
// This provides a threadsafe variant of the filter:tee.
//
// #filter:distributor
//
// A replacement for the filter:threader.

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

class ML_PLUGIN_DECLSPEC input_pusher : public input_type
{
	public:
		input_pusher( ) 
		: prop_length_( pl::pcos::key::from_string( "length" ) )
		, prop_queue_( pl::pcos::key::from_string( "queue" ) )
		, prop_timeout_( pl::pcos::key::from_string( "timeout" ) )
		{ 
			properties( ).append( prop_length_ = INT_MAX );
			properties( ).append( prop_queue_ = 50 );
			properties( ).append( prop_timeout_ = 5000 );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		// Basic information
		virtual const std::wstring get_uri( ) const { return L"nudger:"; }
		virtual const std::wstring get_mime_type( ) const { return L""; }
		virtual bool is_seekable( ) const { return true; }

		// Audio/Visual
		virtual int get_frames( ) const { return prop_length_.value< int >( ); }

		// Push method
		virtual bool push( frame_type_ptr frame )
		{
			lru_.resize( prop_queue_.value< int >( ) );
			if ( frame )
				lru_.append( frame->get_position( ), frame );
			else
				lru_.clear( );
			return true;
		}

	protected:
		// Fetch method
		void do_fetch( frame_type_ptr &result )
		{
			result = lru_.wait( get_position( ), boost::posix_time::milliseconds( prop_timeout_.value< int >( ) ));
			if ( result )
				result = result->shallow( );
		}

	private:
		pl::pcos::property prop_length_;
		pl::pcos::property prop_queue_;
		pl::pcos::property prop_timeout_;
		ml::lru_frame_type lru_;
};

class ML_PLUGIN_DECLSPEC filter_lock : public filter_simple
{
	public:
		filter_lock( )
		: prop_sync_( pl::pcos::key::from_string( "sync" ) )
		, prop_queue_( pl::pcos::key::from_string( "queue" ) )
		, prop_image_( pl::pcos::key::from_string( "image" ) )
		, frames_( 0 )
		{
			properties( ).append( prop_sync_ = 1 );
			properties( ).append( prop_queue_ = 50 );
			properties( ).append( prop_image_ = 0 );
		}

		virtual bool requires_image( ) const { return false; }

		virtual const std::wstring get_uri( ) const { return std::wstring( L"lock" ); }

		virtual const size_t slot_count( ) const { return 1; }

		// Thread safe seek method - each thread holds its own state here
		virtual void seek( const int position, const bool relative = false )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			if ( position_.get( ) == 0 ) position_.reset( new int( 0 ) );
			int *current = position_.get( );
			*current = cl::utilities::clamp( relative ? *current + position : position, 0, int( get_frames( ) - 1 ) );
		}

		// Return the position associated to the thread which this method is running on
		virtual int get_position( ) const
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			return position_.get( ) ? *( position_.get( ) ) : 0;
		}

		// Return the number of frames
		int get_frames( ) const
		{  
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			return frames_;
		}

		// Thread safe connect method
		bool connect( input_type_ptr input, size_t slot = 0 ) 
		{ 
			boost::recursive_mutex::scoped_lock lock( mutex_ ); 
			return filter_type::connect( input, slot ); 
		}

		// Thread safe slot change method
		void on_slot_change( input_type_ptr input, int )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ ); 
			if ( input )
			{
				input->sync( );
				frames_ = input->get_frames( );
			}
			else
			{
				frames_ = 0;
			}
		}

		// Thread safe fetch slot method
		input_type_ptr fetch_slot( size_t slot = 0 ) const
		{   
			boost::recursive_mutex::scoped_lock lock( mutex_ ); 
			return filter_type::fetch_slot( slot ); 
		}

		// The existence of this filter makes the connected graph thread safe
		virtual bool is_thread_safe( )
		{ 
			return true;
		}

		// Sync the frame count - in the case of a downstream threader with multiple
		// threads, we need to sync all duplicated graphs, but if we do a normal sync
		// there's no way to guarantee that they'll all see the same frame count.
		// To resolve this, the threader should set the sync property to 1 on each 
		// immediate upstream lock filter before the first sync call, and set it to
		// 0 before syncing the remainder. 
		void sync( )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ ); 
			if ( prop_sync_.value< int >( ) && fetch_slot( ) )
			{
				fetch_slot( )->sync( );
				frames_ = fetch_slot( )->get_frames( );
			}
		}

		// Threadsafe fetch of a frame for the position associated to the thread
		virtual frame_type_ptr fetch( )
		{
			ml::frame_type_ptr frame;

			// Fetch the image with the lock in place
			{
				boost::recursive_mutex::scoped_lock lock( mutex_ );
				frame = filter_type::fetch( );
			}

			// Clone image if required
			if ( prop_image_.value< int >( ) == 1 )
			{
				il::image_type_ptr image = frame->get_image( );
				frame->set_image( image ? il::image_type_ptr( static_cast< il::image_type * >( image->clone( il::writable ) ) ) : image );
			}

			return frame;
		}

	protected:
		// Invoked by fetch to obtain the frame requested
		void do_fetch( frame_type_ptr &frame )
		{
			// Make sure the lru size matches the requested queue
			lru_.resize( prop_queue_.value< int >( ) );

			// Check if it's in the lru cache first
			frame = lru_.fetch( get_position( ) );

			// If we don't have a frame, attempt to fetch it now
			if ( !frame )
				frame = fetch_from_slot( );

			// If we don't have a frame now, things have got messed up
			ARENFORCE_MSG( frame, "Unable to obtain frame %d from locked input" )( get_position( ) );

			// Finally save the frame in the lru (regardless of whether it's there or not)
			lru_.append( get_position( ), frame );

			// Create a shallow copy
			frame = frame->shallow( );
		}

	private:
		boost::thread_specific_ptr< int > position_;
		mutable boost::recursive_mutex mutex_;
		pl::pcos::property prop_sync_;
		pl::pcos::property prop_queue_;
		pl::pcos::property prop_image_;
		int frames_;
		ml::lru_frame_type lru_;
};

class ML_PLUGIN_DECLSPEC filter_voodoo : public filter_type
{
	public:
		filter_voodoo( )
		: prop_queue_( pl::pcos::key::from_string( "queue" ) )
		, changed_( false )
		, frames_( 0 )
		{
			properties( ).append( prop_queue_ = 50 );
		}

		virtual bool requires_image( ) const { return false; }

		virtual const std::wstring get_uri( ) const { return std::wstring( L"voodoo" ); }

		virtual const size_t slot_count( ) const { return 3; }

		void on_slot_change( input_type_ptr input, int slot = 0 ) { changed_ = true; }

		int get_frames( ) const { return frames_; }

		void sync( )
		{
			refresh_graphs( );

			if ( muxer_ && lock_ )
			{
				muxer_->sync( );
				frames_ = muxer_->get_frames( ); 
			}
			else
			{
				frames_ = 0;
			}
		}

	protected:
		// Invoked by fetch to obtain the frame requested
		void do_fetch( frame_type_ptr &frame )
		{
			// Refresh the graphs if necessary
			refresh_graphs( );

			// Check that we have a muxer to communicate with
			ARENFORCE_MSG( muxer_, "Was unable to derive the muxer" );

			// Seek on the muxer
			muxer_->seek( get_position( ) );

			// Fetch from the muxer
			frame = muxer_->fetch( );
		}

		void refresh_graphs( )
		{
			if ( changed_ )
			{
				// Wipe the old lock and muxer now
				muxer_ = ml::input_type_ptr( );
				lock_ = ml::input_type_ptr( );

				// Fetch the 3 inputs
				ml::input_type_ptr input = fetch_slot( 0 );
				ml::input_type_ptr video = fetch_slot( 1 );
				ml::input_type_ptr audio = fetch_slot( 2 );

				// Sanitise the inputs now
				ARENFORCE_MSG( input, "Nothing connected on the input" );
				ARENFORCE_MSG( video, "Nothing connected on the video slot" );
				ARENFORCE_MSG( audio, "Nothing connected on the audio slot" );

				// Create the lock if necessary
				if ( input->get_uri( ) != L"lock" )
				{
					lock_ = ml::create_filter( L"lock" );
					pl::pcos::assign< std::wstring >( lock_->properties( ), ml::keys::serialise_as, L"" );
					lock_->property( "queue" ) = prop_queue_.value< int >( );
					lock_->connect( input );
				}
				else
				{
					lock_ = input;
				}

				// Replace the nudgers with the the locked input
				video = replace_with_lock( video, 1 );
				audio = replace_with_lock( audio, 2 );

				// Everything should be connected now, so we'll grab a frame from the video and conform audio to that frame rate
				ml::frame_type_ptr v_frame = video->fetch( );
				ml::frame_type_ptr a_frame = audio->fetch( );

				// Ensure that the frame is accesible
				ARENFORCE_MSG( v_frame, "No frame from video slot" );
				ARENFORCE_MSG( a_frame, "No frame from audio slot" );

				// Create the frame_rate filter for the audio
				if ( v_frame->get_fps_num( ) != a_frame->get_fps_num( ) || v_frame->get_fps_den( ) != a_frame->get_fps_den( ) )
				{
					ml::filter_type_ptr audio_fps = ml::create_filter( L"frame_rate" );
					pl::pcos::assign< std::wstring >( audio_fps->properties( ), ml::keys::serialise_as, L"" );
					audio_fps->property( "fps_num" ) = v_frame->get_fps_num( );
					audio_fps->property( "fps_den" ) = v_frame->get_fps_den( );
					audio_fps->connect( audio );
					audio = audio_fps;
				}

				// Make sure that the rest of the world sees the correct graphs
				connect( video, 1 );
				connect( audio, 2 );

				// Construct the muxer
				muxer_ = ml::create_filter( L"muxer" );
				muxer_->property( "use_longest" ) = 0;
				muxer_->connect( video, 0 );
				muxer_->connect( audio, 1 );

				// Sync the muxer
				muxer_->sync( );

				// Everything should be done now
				changed_ = false;
			}
		}

		// Walk the graph, replacing nudgers with lock conform sub graphs
		ml::input_type_ptr replace_with_lock( ml::input_type_ptr &input, int voodoo )
		{
			ml::input_type_ptr result = input;

			if ( input->get_uri( ) == L"nudger:" )
			{
				// TODO: Mark conform filter so that it gets reported as nudger: in an aml dump
				ml::filter_type_ptr conform = ml::create_filter( L"conform" );
				pl::pcos::assign< std::wstring >( conform->properties( ), ml::keys::serialise_as, L"nudger:" );
				conform->property( "image" ) = voodoo == 1 ? 1 : 0;
				conform->property( "audio" ) = voodoo == 1 ? 0 : 1;
				conform->connect( lock_ );
				result = conform;
			}
			else
			{
				for ( size_t i = 0; i < input->slot_count( ); i ++ )
				{
					ml::input_type_ptr slot = input->fetch_slot( i );
					if ( slot )
					{
						ml::input_type_ptr upstream = replace_with_lock( slot, voodoo );
						if ( upstream != slot )
							input->connect( upstream, i );
					}
				}
			}

			return result;
		}

	private:
		pl::pcos::property prop_queue_;
		ml::input_type_ptr lock_;
		ml::input_type_ptr muxer_;
		bool changed_;
		int frames_;
};

class ML_PLUGIN_DECLSPEC filter_fork : public filter_type
{
	public:
		filter_fork( )
		: prop_slots_( pl::pcos::key::from_string( "slots" ) )
		, prop_queue_( pl::pcos::key::from_string( "queue" ) )
		, prop_preroll_( pl::pcos::key::from_string( "preroll" ) )
		, prop_solo_( pl::pcos::key::from_string( "solo" ) )
		, prop_image_( pl::pcos::key::from_string( "image" ) )
		, changed_( false )
		, frames_( 0 )
		{
			properties( ).append( prop_slots_ = 2 );
			properties( ).append( prop_queue_ = 50 );
			properties( ).append( prop_preroll_ = 25 );
			properties( ).append( prop_solo_ = 0 );
			properties( ).append( prop_image_ = 0 );
		}

		virtual bool requires_image( ) const { return false; }

		virtual const std::wstring get_uri( ) const { return std::wstring( L"fork" ); }

		virtual const size_t slot_count( ) const { return prop_slots_.value< int >( ); }

		void on_slot_change( input_type_ptr input, int slot = 0 ) 
		{
			if ( slot == 0 && input )
			{
				seek( input->get_position( ) );
			}
			changed_ = true; 
		}

		int get_frames( ) const { return frames_; }

		void sync( )
		{
			refresh_graphs( );

			if ( lock_ )
			{
				lock_->sync( );
				frames_ = lock_->get_frames( ); 
				for( size_t i = 1; i < slot_count( ); i ++ )
					fetch_slot( i )->sync( );
			}
			else
			{
				frames_ = 0;
			}
		}

	protected:
		// Invoked by fetch to obtain the frame requested
		void do_fetch( frame_type_ptr &frame )
		{
			// Refresh the graphs if necessary
			refresh_graphs( );

			// Check that we have a muxer to communicate with
			ARENFORCE_MSG( lock_, "Was unable to derive the lock" );

			int solo = prop_solo_.value< int >( );

			if ( solo <= 0 || solo >= int( slot_count( ) ) )
			{
				lock_->seek( get_position( ) );
				frame = lock_->fetch( );

				for( size_t i = 1; i < slot_count( ); i ++ )
				{
					fetch_slot( i )->seek( get_position( ) );
					fetch_slot( i )->fetch( );
				}
			}
			else
			{
				fetch_slot( solo )->seek( get_position( ) );
				frame = fetch_slot( solo )->fetch( );
			}
		}

		void refresh_graphs( )
		{
			if ( changed_ )
			{
				// Wipe the old lock now
				lock_ = ml::input_type_ptr( );

				// Fetch the 3 inputs
				ml::input_type_ptr input = fetch_slot( 0 );

				// Sanitise the inputs now
				ARENFORCE_MSG( input, "Nothing connected on the input" );

				// Create the lock if necessary
				if ( input->get_uri( ) != L"lock" )
				{
					lock_ = ml::create_filter( L"lock" );
					pl::pcos::assign< std::wstring >( lock_->properties( ), ml::keys::serialise_as, L"nudger:" );
					lock_->property( "queue" ) = prop_queue_.value< int >( );
					lock_->property( "image" ) = prop_image_.value< int >( );
					lock_->connect( input );
					lock_->seek( get_position( ) );
				}
				else
				{
					lock_ = input;
				}

				// Grab a frame from the lock so that we know the frame 
				ml::frame_type_ptr frame = lock_->fetch( get_position( ) );

				// Replace the nudgers with the the locked input
				for( size_t i = 1; i < slot_count( ); i ++ )
				{
					ml::input_type_ptr slot = fetch_slot( i );

					ml::input_type_ptr graph = replace_with_lock( slot );
					graph->sync( );

					ml::frame_type_ptr temp = graph->fetch( get_position( ) );
					if ( temp->get_fps_num( ) != frame->get_fps_num( ) || temp->get_fps_den( ) != frame->get_fps_den( ) )
					{
						ml::filter_type_ptr fps = ml::create_filter( L"frame_rate" );
						pl::pcos::assign< std::wstring >( fps->properties( ), ml::keys::serialise_as, L"" );
						fps->property( "fps_num" ) = frame->get_fps_num( );
						fps->property( "fps_den" ) = frame->get_fps_den( );
						fps->connect( graph );
						fps->seek( get_position( ) );
						graph->sync( );
						graph = fps;
					}

					if ( graph != slot )
						connect( graph, i );
				}

				// Everything should be done now
				changed_ = false;
			}
		}

		// Walk the graph, replacing nudgers with lock conform sub graphs
		ml::input_type_ptr replace_with_lock( ml::input_type_ptr &input )
		{
			ml::input_type_ptr result = input;

			if ( input->get_uri( ) == L"nudger:" )
			{
				// TODO: Mark lock filter so that it gets reported as nudger: in an aml dump
				result = lock_;
			}
			else
			{
				for ( size_t i = 0; i < input->slot_count( ); i ++ )
				{
					ml::input_type_ptr slot = input->fetch_slot( i );
					if ( slot )
					{
						ml::input_type_ptr upstream = replace_with_lock( slot );
						if ( upstream != slot )
							input->connect( upstream, i );
						if ( input->get_uri( ) == L"voodoo" || input->get_uri( ) == L"fork" )
							break;
					}
				}
			}

			return result;
		}

	private:
		pl::pcos::property prop_slots_;
		pl::pcos::property prop_queue_;
		pl::pcos::property prop_preroll_;
		pl::pcos::property prop_solo_;
		pl::pcos::property prop_image_;
		ml::input_type_ptr lock_;
		bool changed_;
		int frames_;
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

class ML_PLUGIN_DECLSPEC filter_threader : public filter_simple
{
	public:
		filter_threader( )
		: obs_active_( new observer< filter_threader >( this, &filter_threader::update_active ) )
		, last_used_( boost::get_system_time( ) )
		, prop_threads_( pl::pcos::key::from_string( "threads" ) )
		, prop_queue_( pl::pcos::key::from_string( "queue" ) )
		, prop_active_( pl::pcos::key::from_string( "active" ) )
		, prop_audio_direction_( pl::pcos::key::from_string( "audio_direction" ) )
		, prop_trigger_( pl::pcos::key::from_string( "trigger" ) )
		, prop_timeout_( pl::pcos::key::from_string( "timeout" ) )
		, monitor_( cl::function_job_ptr( new cl::function_job( boost::bind( &filter_threader::monitor, this, 5 ) ) ) )
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

		virtual ~filter_threader( )
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
				cl::function_job_ptr fjob( new cl::function_job( boost::bind( &filter_threader::decode_job, this, position ) ) );
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

//
// Plugin object
//

class ML_PLUGIN_DECLSPEC plugin : public openmedialib_plugin
{
public:
	virtual input_type_ptr input( const std::wstring & )
	{
		return input_type_ptr( new input_pusher( ) );
	}

	virtual filter_type_ptr filter( const std::wstring &spec )
	{
		if ( spec == L"fork" )
			return filter_type_ptr( new filter_fork( ) );
		if ( spec == L"lock" )
			return filter_type_ptr( new filter_lock( ) );
		if ( spec == L"distributor" )
			return filter_type_ptr( new filter_threader( ) );
		if ( spec == L"voodoo" )
			return filter_type_ptr( new filter_voodoo( ) );
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
