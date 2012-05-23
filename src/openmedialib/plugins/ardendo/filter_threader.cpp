// Threading filter
//
// Copyright (C) 2008 Ardendo
// Released under the terms of the LGPL.
//
// #filter:threader
// 
// This filter provides a read ahead component into a graph.
//
// Important notes about changing the graph that's connected to this filter:
// * The filter must be paused (by setting the active property to 0) before
//   any modification to the connected graph is done.
// * If the modification of the connected graph causes the duration of it
//   to shrink, sync() must be called on the filter before it is resumed.
//
//
//Internal threading rules for this filter:
//
//* All change of worker thread state must be through the
//  apply_new_thread_state() method. The state_ and new_state_
//  member variables may not be changed anywhere else.
//
//* The main thread may only access the connected input
//  if state_ is set to thread_dead or thread_paused.
//	If state_ is thread_running, only the worker thread
//  may access the connected input.
//
//* When doing potentially lengthy operations such as fetch()
//  and get_image(), the worker thread must unlock the mutex
//  to allow the main thread to retrieve frames from the queue
//  in the meantime.
//
//* The worker thread must call accept_new_thread_state() at
//  least once between every fetch in order for the state 
//  change operations on the main thread to remain responsive.
//
//* The main thread may never perform any non-const operations
//  on the the cache_ member (outside of the class constructor).
//  Only the background thread may do this.

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"

#include <opencorelib/cl/thread_name.hpp>

#include <map>

#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/foreach.hpp>

#include <iostream>

#ifdef WIN32
#pragma warning(push)
#pragma warning(disable:4355)
#endif

namespace aml { namespace openmedialib {

static pl::pcos::key key_audio_reversed_ = pl::pcos::key::from_string( "audio_reversed" );

#define const_threader const_cast< filter_threader * >

//The 3 mutually exclusive states of the worker thread:
// * thread_dead: The thread does not exist
// * thread_running: The thread is running, caching frames
// * thread_paused: The thread exists, but is paused
enum thread_state
{
	thread_dead,
	thread_running,
	thread_paused
};

const char *state_names[] = { "dead", "running", "paused" };

//The time, in milliseconds, between each sync call on the background thread.
const int TIME_BETWEEN_BACKGROUND_SYNCS = 1000;

//Returns a string describing what's in the given threader cache
std::string cache_to_string( const std::map< int, ml::frame_type_ptr >& cache )
{
	std::map< int, ml::frame_type_ptr >::const_iterator it = cache.begin( );
	std::map< int, ml::frame_type_ptr >::const_iterator eit = cache.end( );
	
	std::string out;
	
	for( ;it != eit; ++it )
	{
		out += boost::lexical_cast< std::string >( it->first );
		out += std::string( " " );
	}
	
	return out;
}

class ML_PLUGIN_DECLSPEC filter_threader : public ml::filter_type
{
	typedef boost::recursive_mutex::scoped_lock scoped_lock;

	public:
		filter_threader( )
			: ml::filter_type( )
			, prop_active_( pl::pcos::key::from_string( "active" ) )
			, obs_active_( new fn_observer< filter_threader >( const_threader( this ), &filter_threader::update_active ) )
			, prop_queue_( pl::pcos::key::from_string( "queue" ) )
			, obs_queue_( new fn_observer< filter_threader >( const_threader( this ), &filter_threader::update_queue ) )
			, active_( 0 )
			, input_is_thread_safe_( false )
			, state_( thread_dead )
			, new_state_( thread_dead )
			, prop_audio_direction_( pl::pcos::key::from_string( "audio_direction" ) )
			, last_position_( -1 )
			, frames_( 0 )
			, reader_( )
			, speed_( 1 )
			, position_( 0 )
			, sync_time_( boost::get_system_time( ) )
			, last_sync_count_( 0 )
			, next_cache_size_report_( boost::get_system_time( ) + boost::posix_time::seconds( 1 ) )
		{
			ARLOG_INFO( "Creating threader filter (this = 0x%|x|)" )( reinterpret_cast< uintptr_t >( this ) );

			properties( ).append( prop_active_ = 0 );
			properties( ).append( prop_queue_ = 25 );
			properties( ).append( prop_audio_direction_ = 1 );
			prop_active_.attach( obs_active_ );
			prop_queue_.attach( obs_queue_ );
			max_cache_size_ = prop_queue_.value<int>();
		}

		virtual ~filter_threader( )
		{ 
			scoped_lock lock( mutex_ );
			stop( lock );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return prop_active_.value< int >( ) != 0; }

		virtual const pl::wstring get_uri( ) const 
		{ return L"threader"; }

		int get_frames() const
		{  
			scoped_lock lock( mutex_ );
			return frames_;
		}

		int get_position( ) const 
		{
			scoped_lock lock( mutex_ ); 
			return position_;
		}

		bool connect( ml::input_type_ptr input, size_t slot = 0 ) 
		{ 
			scoped_lock lock( mutex_ ); 
			return filter_type::connect( input, slot ); 
		}

		void seek( const int position, const bool relative ) 
		{ 
			scoped_lock lock( mutex_ ); 

			if ( relative )
				position_ += position;
			else
				position_ = position;

			if ( position_ < 0 )
				position_ = 0;
			else if ( position_ >= frames_ )
				position_ = frames_ - 1;

			bool changed = position != last_position_;

			if ( changed )
			{
				speed_ = position - last_position_;

				last_position_ = position;
			}

			//Notify the worker thread about the position/speed change
			cond_.notify_one();
		}

		void on_slot_change( ml::input_type_ptr input, int )
		{
			scoped_lock lock( mutex_ ); 
			ARENFORCE_MSG( state_ != thread_running, "Threader filter must be paused when changing connected filters!" );
			input_is_thread_safe_ = input && is_thread_safe( );
		}

		ml::input_type_ptr fetch_slot( size_t slot ) const
		{   
			scoped_lock lock( mutex_ ); 
			return filter_type::fetch_slot( slot ); 
		}

		void sync( )
		{
			scoped_lock lck( mutex_ );

			check_for_worker_error( lck, true );

			if( state_ == thread_dead || state_ == thread_paused )
			{
				ml::input_type_ptr input = fetch_slot( 0 );
				if ( input )
				{
					input->sync( );
					ARENFORCE_MSG( input->get_frames() > 0,
						"Media length was 0 after sync! (Previous length was %1%)" )
						( frames_ )( input->get_uri() );
					last_sync_count_ = frames_ = input->get_frames( );
				}
				else
				{
					last_sync_count_ = frames_ = 0;
				}
			}
			else
			{
				//The prefetch thread is running, so it takes care of
				//syncing. Update with the value from the latest sync
				//that it made.
				frames_ = last_sync_count_;
			}
		}

	protected:

		void do_fetch( ml::frame_type_ptr &result )
		{
			scoped_lock lck( mutex_ );

			check_for_worker_error( lck, true );

			ml::input_type_ptr input = fetch_slot( 0 );

			if( input )
			{
				int position = get_position( );

				if( state_ == thread_paused || state_ == thread_dead )
				{
					ARLOG_IF_ENV( "AMF_PERFORMANCE_LOGGING",
						"Requesting frame %1% from threader filter with state %2%." )
						( position )( state_names[ state_ ] );

					input->seek( position );
					result = input->fetch( );

					if( result && !result->in_error() && state_ == thread_dead && active_ == 1 )
					{
						//The active property is set to running, but the thread state is dead. This means
						//that the worker thread exited earlier due to a failure of some kind (such as a
						//network disconnect).
						//Since the state of the graph appears to be OK again, we attempt to restart the
						//worker thread now.
						ARLOG_INFO("Fetch of frame %1% was successful. Will restart the threader worker again, due to the active property being %2%")
							( position )( active_ );
						update_active( lck );
					}
				}
				else
				{
					ARLOG_IF_ENV( "AMF_PERFORMANCE_LOGGING", 
						"Requesting frame %1% from active threader filter." )
						( position );

					//The frame will be inserted into the queue eventually,
					//so just wait for it.
					if( !( result = cached( position, lck ) ) )
					{
						ARLOG_IF_ENV( "AMF_PERFORMANCE_LOGGING", 
							"Frame %1% not found in cache. Waiting.\nCache is: %2%" )
							( position )( cache_to_string( cache_ ) );

						do 
						{
							cond_.wait( lck );
							//Check for error in the worker thread, and rethrow if so
							check_for_worker_error( lck, true );
						} while( !( result = cached( position, lck ) ) );

						ARLOG_IF_ENV( "AMF_PERFORMANCE_LOGGING", 
							"Got frame %1% after waiting." )
							( position );
					}
					else
					{
						ARLOG_IF_ENV( "AMF_PERFORMANCE_LOGGING", 
							"Frame %1% was cached." )
							( position );
					}
				}

				// Create a shallow copy to ensure the reference and cache are unaffected later in the graph
				result = result->shallow( );

				// Deal with audio direction
				handle_reverse_audio( result );
			}
		}

		void handle_reverse_audio( ml::frame_type_ptr result )
		{
			if ( result && prop_audio_direction_.value< int >( ) )
			{
				if ( result->properties( ).get_property_with_key( key_audio_reversed_ ).valid( ) )
				{
					pl::pcos::property audio_reversed = result->properties( ).get_property_with_key( key_audio_reversed_ );
					if ( ( audio_reversed.value< int >( ) && speed_ >= 0 ) || ( !audio_reversed.value< int >( ) && speed_ < 0 ) )
					{
						result->set_audio( ml::audio::reverse( result->get_audio( ) ) );
						audio_reversed = audio_reversed.value< int >( ) ? 0 : 1;
					}
				}
				else
				{
					pl::pcos::property audio_reversed( key_audio_reversed_ );
					if ( speed_ < 0 )
					{
						result->set_audio( ml::audio::reverse( result->get_audio( ) ) );
					}
					result->properties( ).append( audio_reversed = speed_ < 0 ? 1 : 0 );
				}
			}
		}

		void apply_new_thread_state( thread_state the_state, scoped_lock& lck )
		{
			//Set the desired new state for the worker and notify the
			//worker.
			new_state_ = the_state;
			cond_.notify_one();

			//Wait for the worker to accept the state change
			ARLOG_IF_ENV( "AMF_PERFORMANCE_LOGGING",
				"Waiting for worker to accept new state: \"%1%\"" )
				( state_names[ new_state_ ] );
			do 
			{
				cond_.wait( lck );
				if( check_for_worker_error( lck, false ) )
					return;

			} while ( state_ != new_state_ );

			ARLOG_IF_ENV( "AMF_PERFORMANCE_LOGGING",
				"Worker accepted new state: \"%1%\"" )
				( state_names[ state_ ] );

			if( state_ == thread_dead )
			{
				ARLOG_IF_ENV( "AMF_PERFORMANCE_LOGGING",
					"Waiting for worker thread to exit." );

				//If we signaled the thread to shut down,
				//we join with it here
				reader_.join();

				ARLOG_IF_ENV( "AMF_PERFORMANCE_LOGGING",
					"Worker thread exited." );
			}
		}

		bool check_for_worker_error( scoped_lock &lck, bool rethrow )
		{
			if( !thread_error_msg_.empty() )
			{
				ARLOG_ERR( "Threader worker terminated with error:\n%1%" )( thread_error_msg_ );
				olib::t_string err_msg_copy = thread_error_msg_;
				thread_error_msg_.clear();
				state_ = new_state_ = thread_dead;
				reader_.join();

				if( rethrow )
				{
					ARENFORCE_MSG( false, _CT( "Threader worker error:\n%1%" ) )( err_msg_copy );
				}

				return true;
			}

			return false;
		}

		void start( scoped_lock& lck  )
		{
			if ( input_is_thread_safe_ )
			{
				check_for_worker_error( lck, false );

				//The thread is already running. We don't need to do anything.
				if( state_ == thread_running )
				{
					return;
				}
				else
				{
					if ( state_ == thread_dead )
					{
						thread_error_msg_.clear();
						//Create the worker thread.
						reader_ = boost::thread( boost::bind( &filter_threader::run, this ) );
					}

					//The thread was either paused or did not exist. In any case, start
					//it now.
					apply_new_thread_state( thread_running, lck );
				}
			}
		}

		void pause( scoped_lock& lck )
		{
			if( input_is_thread_safe_ )
			{
				check_for_worker_error( lck, false );

				if( state_ == thread_running )
				{
					apply_new_thread_state( thread_paused, lck );
				}
				else if( state_ == thread_dead )
				{
					start( lck );
					apply_new_thread_state( thread_paused, lck );
				}
			}
		}

		void stop( scoped_lock &lck )
		{
			if ( input_is_thread_safe_ )
			{
				check_for_worker_error( lck, false );

				if ( state_ != thread_dead )
				{
					apply_new_thread_state( thread_dead, lck );
				}
			}
		}

		//Called when the "active" property changes.
		//Important: Never call this from inside the filter with the lock held.
		//Use the overload which takes a lock instead.
		void update_active( )
		{
			scoped_lock lck( mutex_ );
			update_active( lck );
		}

		void update_active( scoped_lock &lck )
		{
			if ( input_is_thread_safe_ )
			{
				active_ = prop_active_.value< int >( );

				if ( active_ == 0 )
					pause( lck );
				else if ( active_ == 1 )
					start( lck );
				else if ( active_ == 2 )
				{
					//2 previously meant pause, and 0 meant stop, so
					//we accept 2 here as well for legacy reasons.
					pause( lck );
				}
			}
			else
			{
				prop_active_ = 0;
				state_ = thread_dead;
			}
		}

		//Called when the queue property changes.
		//Important: Never call this from inside the filter with
		//the lock held.
		void update_queue( )
		{
			scoped_lock lck( mutex_ );
			pause( lck );

			max_cache_size_ = prop_queue_.value<int>();

			//Reapply the current state, i.e. if we we're running
			//before, we should be running afterwards also.
			update_active( lck );
		}

		ml::frame_type_ptr cached( int position, scoped_lock& lck )
		{
			std::map< int, ml::frame_type_ptr >::iterator it = cache_.find( position );
			return it != cache_.end( ) ? it->second : ml::frame_type_ptr( );
		}

		void background_sync( scoped_lock &lck )
		{
			//State sanity check
			ARENFORCE( state_ != thread_dead );

			//Is it time for a new sync?
			if( boost::get_system_time( ) >= sync_time_ )
			{
				ml::input_type_ptr input = fetch_slot( 0 );
				ARENFORCE( input );

				int frames = last_sync_count_;

				{
					//Unlock while performing sync, since it is potentially
					//time-consuming
					lck.unlock();
					ARGUARD( boost::bind( &scoped_lock::lock, &lck ) );					

					input->sync( );

					frames = input->get_frames( );
				}
				
				sync_time_ = boost::get_system_time( ) + boost::posix_time::milliseconds( TIME_BETWEEN_BACKGROUND_SYNCS );
				
				//The input should never shrink on a background sync. If it did, it's for one of these reasons:
				//* A connection failure of some kind, so that the actual input shrunk
				//* The graph was changed so that the duration shrunk while the thread was paused,
				//  but sync() was not called on the filter before it was resumed.
				//* The connected input was incorrectly modified while the thread was running.
				ARENFORCE_MSG( frames >= last_sync_count_,
					"Media shrunk on last background sync! Previous length was %1%, new length is %2%" )
					( last_sync_count_ )( frames )( input->get_uri() );

				last_sync_count_ = frames;
			}
		}

		void accept_new_state( scoped_lock &lck )
		{
			if( new_state_ != state_ )
			{
				ARLOG_IF_ENV( "AMF_PERFORMANCE_LOGGING", 
					"Worker accepting new state: \"%1%\" (previous state was: \"%2%\")." )
					( state_names[ new_state_ ] )( state_names[ state_ ] );

				//Apply the new state and notify the main thread
				//that we've done so.
				state_ = new_state_;
				cond_.notify_one();
			}
		}

		//Removes all frames 
		void scrub_cache( scoped_lock &lck, int position, int speed )
		{
			const int abs_speed = abs( speed );

			typedef std::map< int, ml::frame_type_ptr >::iterator iter_type;

			std::list< iter_type > remove_list;
			for( iter_type it = cache_.begin(); it != cache_.end(); ++it )
			{
				int relative_to_pos = ( it->first - position );
				if( relative_to_pos % abs_speed != 0 )
				{
					remove_list.push_back( it );
				}
			}

			BOOST_FOREACH( iter_type &it, remove_list )
			{
				cache_.erase( it );
			}
		}

		//Looks in the cache for a suitable frame to remove, given the current
		//position and speed (i.e. frame interval between each fetch).
		bool remove_frame_from_cache( scoped_lock &lck, int position, int speed )
		{
			typedef std::map< int, ml::frame_type_ptr >::iterator iter_type;
			ARENFORCE( cache_.size() > 0 );
			ARENFORCE( speed != 0);

			const int abs_speed = abs( speed );
			const int TAIL_FRACTION = 4;

			const int tail = ( max_cache_size_ / TAIL_FRACTION ) * abs_speed;
			const int head1 = ( max_cache_size_ - 1 ) * abs_speed - tail;
			const int head2 = ( max_cache_size_ - 1 ) * abs_speed;
			int lowest_required, highest_required, lowest_permitted, highest_permitted;
			if( speed > 0 )
			{
				lowest_required = position;
				highest_required = position + head1;
				lowest_permitted = position - tail;
				highest_permitted = position + head2;
			}
			else
			{
				lowest_required = position - head1;
				highest_required = position;
				lowest_permitted = position - head2;
				highest_permitted = position + tail;
			}

			iter_type to_remove = cache_.end();
			iter_type min = cache_.begin();
			iter_type max = cache_.end();
			--max;

			if( min->first < lowest_permitted )
				to_remove = min;
			else if( max->first > highest_permitted )
				to_remove = max;
			else
			{
				iter_type it1 = cache_.find( lowest_required );
				iter_type it2 = cache_.find( highest_required );

				if( it1 == cache_.end() || it2 == cache_.end() || it2->first - it1->first < head1 )
				{
					//Some required frames are missing from the cache,
					//but it only contains permitted frames.
					if( min->first < lowest_required )
						to_remove = min;
					else if( max->first > highest_required )
						to_remove = max;
				}
				else
				{
					ARENFORCE( it2->first - it1->first == head1 );
					//Only permitted frames in cache, and all the
					//required frames are there. The cache is now
					//in a good state, and we shouldn't attempt to
					//remove anything from it
					return false;
				}
			}

			ARENFORCE( to_remove != cache_.end() );
			ARLOG_IF_ENV( "AMF_PERFORMANCE_LOGGING", "Removing %1% from cache." )( to_remove->first );
			cache_.erase( to_remove );

			return true;
		}

		void run( )
		{
			scoped_lock lck( mutex_ );

			ARASSERT( thread_error_msg_.empty() );

			try
			{
				// Set the thread name to simplify debugging
				olib::t_format thread_name_format = 
					olib::t_format( _CT("Threader filter worker for 0x%|x|)") ) % reinterpret_cast< uintptr_t >( this );
				olib::opencorelib::set_thread_name( thread_name_format.str() );

				sync_time_ = boost::get_system_time() + boost::posix_time::milliseconds( TIME_BETWEEN_BACKGROUND_SYNCS );

				int prefetch_position = 0;
				bool fully_cached = false;

				//Keep thread local copies of the position_ and speed_ members.
				//This is needed to ensure that they do not change unexpectedly,
				//since we unlock the mutex during lengthy operations in the code
				//below.
				int current_filter_position = position_;
				int current_filter_speed = speed_;
				
				cache_.clear();

				while( true )
				{
					//First of all, we handle any new thread state imposed upon us
					//by the main thread.
					while( true )
					{
						//Check if the main thread wants us to change state
						accept_new_state( lck );
						
						if( state_ == thread_dead )
						{
							//Main thread signaled that this thread should quit and is waiting
							//in a join() call.
							return;
						}

						if( state_ == thread_paused )
						{
							//There might be changes to the input now, so the
							//cached frames are not necessarily valid anymore.
							cache_.clear();
							fully_cached = false;
							prefetch_position = current_filter_position;

							//We're paused. Wait for a state change from the main thread.
							cond_.wait( lck );
						}
						else
						{
							ARENFORCE( state_ == thread_running );

							//If it's time to sync the frame count, do that now
							background_sync( lck );

							if( current_filter_position != position_ || 
								current_filter_speed != speed_ )
							{
								//If the filter position/speed has changed, we should
								//potentially be adding new frames to the cache.
								fully_cached = false;

								if( speed_ != current_filter_speed || position_ != current_filter_position + speed_ )
								{
									//If the speed changed, or the new position is not in the current
									//direction, reset the prefetch position. Otherwise, the main thread
									//is just fetching linearly, and we can leave the prefetch position
									//as it is, since we're guaranteed to have fetched or soon be going
									//to fetch that frame anyway.
									prefetch_position = position_;
									scrub_cache( lck, position_, speed_ );
									ARLOG_IF_ENV( "AMF_PERFORMANCE_LOGGING",
										"Non-linear seek. New position: %1%, new speed: %2%" )
										( position_ )( speed_ );
								}

								current_filter_position = position_;
								current_filter_speed = speed_;
								
								break;
							}
							else if( !fully_cached && prefetch_position >= 0 && prefetch_position < last_sync_count_ )
							{
								//The cache is not filled with optimal frames, and the current
								//prefetch position is valid. We should continue prefetching frames
								//into the cache.
								break;
							}
							else
							{
								//We're running, but our cache is already filled with optimal
								//frames, or we've hit the beginning/end of the media.
								//Wait for position/state change on the main thread, or for
								//the media to grow.

								ARENFORCE( cache_.find( current_filter_position ) != cache_.end() );
								cond_.timed_wait( lck, sync_time_ );
							}
						}
					}

					//State sanity check
					ARENFORCE( state_ == thread_running );

					//Cache size sanity check
					ARENFORCE( cache_.size() <= static_cast< size_t >( max_cache_size_ ) );

					//Is there room in the cache?
					if( cache_.size() == max_cache_size_ )
					{
						if( !remove_frame_from_cache( lck, current_filter_position, current_filter_speed ) )
						{
							//Could not find a suitable frame to remove.
							//This means the cache is now in a good state for this
							//position/speed combination, and we should leave it as is.
							ARENFORCE( cache_.find( current_filter_position ) != cache_.end() );
							fully_cached = true;
							continue;
						}
					}

					//Cache size sanity check
					ARENFORCE( cache_.size() < static_cast< size_t >( max_cache_size_ ) );

					ml::input_type_ptr input = fetch_slot( 0 );
					ARENFORCE( input );

					bool inserted_frame = false;
					while( !inserted_frame && prefetch_position >= 0 && prefetch_position < last_sync_count_ )
					{
						//Check if the frame is in the cache already. If not, fetch it.
						if( cache_.find( prefetch_position ) == cache_.end() )
						{
							ARLOG_IF_ENV( "AMF_PERFORMANCE_LOGGING",
								"Prefetching frame %1% on background thread" )
								( prefetch_position );

							ml::frame_type_ptr frame;
							{
								//Unlock, so that calls to fetch() and seek() on the main thread
								//will not be blocked.
								lck.unlock();
								ARGUARD( boost::bind( &scoped_lock::lock, &lck ) );

								input->seek( prefetch_position );
								frame = input->fetch( );

								//Retrieved frame sanity check
								ARENFORCE( frame )( prefetch_position )( input->get_uri() );
								ARENFORCE( frame->get_position() == prefetch_position )( frame->get_position() )( prefetch_position )( input->get_uri() );

								if ( frame->get_image( ) )
									frame->get_image( )->set_writable( false );
								if ( frame->get_alpha( ) )
									frame->get_alpha( )->set_writable( false );
							}

							cache_[ prefetch_position ] = frame;
							inserted_frame = true;

							ARLOG_IF_ENV( "AMF_PERFORMANCE_LOGGING",
								"Prefetching of frame %1% done." )
								( prefetch_position );

							//Notify the main thread that a new frame has been inserted into the cache
							cond_.notify_one();
						}

						//Advance the prefetch position to the next frame
						prefetch_position += current_filter_speed;
					}
				}
			}
			catch ( std::exception &exc )
			{
				ARLOG_ERR("Caught exception in threader worker:\n%1%")( exc.what() );
				thread_error_msg_ = olib::opencorelib::str_util::to_t_string( exc.what() );
				if( thread_error_msg_.empty() )
					thread_error_msg_ = _CT("Caught empty std-exception in threader worker!");
				cond_.notify_one();
			}
			catch ( ... )
			{
				ARLOG_ERR("Caught unknown (non-std) exception in threader worker!");
				thread_error_msg_ = _CT("Caught unknown (non-std) exception in threader worker!");
				cond_.notify_one();
			}
		}

	private:
		//Control property for the prefetch thread state.
		pl::pcos::property prop_active_;
		boost::shared_ptr< pl::pcos::observer > obs_active_;
		int active_;
		bool input_is_thread_safe_;

		//The maximum size (in number of frames) of the prefetch cache
		pl::pcos::property prop_queue_;
		boost::shared_ptr< pl::pcos::observer > obs_queue_;
		int max_cache_size_;

		//Thread control related
		mutable boost::recursive_mutex mutex_;
		boost::condition_variable_any cond_;
		thread_state state_; //The current state of the worker thread
		thread_state new_state_; //The desired new state that the worker thread should assume
		olib::t_string thread_error_msg_; //If the worker thread encounters an exception, its message is stored here
		
		pl::pcos::property prop_audio_direction_;
		int last_position_;
		int frames_;
		std::map< int, ml::frame_type_ptr > cache_;
		boost::thread reader_;
		int speed_;
		int position_;
		boost::system_time sync_time_;
		int last_sync_count_;
		boost::system_time next_cache_size_report_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_threader( const pl::wstring & )
{
	return ml::filter_type_ptr( new filter_threader( ) );
}

} }

#ifdef WIN32
#pragma warning(pop)
#endif
