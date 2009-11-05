// Threading filter
//
// Copyright (C) 2008 Ardendo
// Released under the terms of the LGPL.
//
// #filter:threader
// 
// This filter provides a read ahead component into a graph.

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"

#include <map>

#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>

#include <iostream>

#ifdef WIN32
#pragma warning(push)
#pragma warning(disable:4355)
#endif

namespace aml { namespace openmedialib {

static pl::pcos::key key_audio_reversed_ = pl::pcos::key::from_string( "audio_reversed" );

#define const_threader const_cast< filter_threader * >

enum thread_state
{
	thread_inactive = 0,
	thread_running = 1,
	thread_paused = 2,
	thread_accepted = 4,
	thread_eof = 8
};

struct do_lock_t
{

};

class ML_PLUGIN_DECLSPEC filter_threader : public ml::filter_type
{
	typedef boost::recursive_mutex::scoped_lock scoped_lock;

	struct reader_thread
	{
		reader_thread( filter_threader *filter ) : filter_( filter ) { }
		void operator( )( ) { filter_->run( do_lock_t() ); }
		filter_threader *filter_;
	};

	public:
		filter_threader( )
			: ml::filter_type( )
			, start_( this )
			, obs_active_( new fn_observer< filter_threader >( const_threader( this ), &filter_threader::update_active ) )
			, prop_active_( pl::pcos::key::from_string( "active" ) )
			, active_( 0 )
			, state_( thread_inactive )
			, prop_queue_( pl::pcos::key::from_string( "queue" ) )
			, prop_audio_direction_( pl::pcos::key::from_string( "audio_direction" ) )
			, last_position_( -1 )
			, mutex_( )
			, cond_( )
			, frames_( 0 )
			, cache_( )
			, reader_( 0 )
			, speed_( 1 )
			, last_frame_( )
			, has_sub_thread_( false )
			, position_( 0 )
		{
			properties( ).append( prop_active_ = 0 );
			properties( ).append( prop_queue_ = 25 );
			properties( ).append( prop_audio_direction_ = 1 );
			prop_active_.attach( obs_active_ );
		}

		virtual ~filter_threader( )
		{ 
			scoped_lock lck( mutex_ );
			stop( lck );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return prop_active_.value< int >( ) != 0; }

		virtual const pl::wstring get_uri( ) const 
		{ return L"threader"; }

        int get_frames() const
        {  
            scoped_lock lock( mutex_ );
            int result = get_frames( lock );
			return result;
        }

		int get_frames( scoped_lock& lck  ) const 
		{
			return fetch_slot( 0, lck ) ? fetch_slot( 0, lck )->get_frames( ) : 0;
		}

        int get_position( scoped_lock& lck  ) const 
        {
            return position_;
        }

		virtual int get_position( ) const 
		{ 
            
            scoped_lock lock( mutex_ ); 
            int result = get_position( lock );
			return result;
        }

        bool connect( ml::input_type_ptr input, size_t slot, scoped_lock& lck ) 
        { 
            
            return filter_type::connect( input, slot ); 
        }

		virtual bool connect( ml::input_type_ptr input, size_t slot = 0 ) 
		{ 
            
            scoped_lock lock( mutex_ ); 
            return connect( input, slot, lock ); 
        }

		void seek( const int position, const bool relative ) 
		{ 
            if ( relative )
				position_ += position;
			else
				position_ = position;

			if ( position_ < 0 )
				position_ = 0;
			else if ( position_ >= get_frames( ) )
				position_ = get_frames( ) - 1;
        }

		void on_slot_change( ml::input_type_ptr input, int, scoped_lock& lck  )
		{
            
			if ( input )
			{
				frames_ = input->get_frames( );
				//has_sub_thread_ = contains_sub_thread( input );
			}
		}

        virtual void on_slot_change( ml::input_type_ptr input, int v )
        {
            
            scoped_lock lock( mutex_ );
            on_slot_change( input, v, lock );
        }

        ml::input_type_ptr fetch_slot( size_t slot, scoped_lock& lck ) const
        {   
            return filter_type::fetch_slot( slot ); 
        }

		virtual ml::input_type_ptr fetch_slot( size_t slot ) const
		{ 
            scoped_lock lock( mutex_ ); 
            return filter_type::fetch_slot( slot ); 
        }

        bool is_thread_safe( scoped_lock& lck )
        { 
            
            return filter_type::is_thread_safe( ); 
        }

		virtual bool is_thread_safe( )
		{ 
            
            scoped_lock lock( mutex_ ); 
            return is_thread_safe( lock ); 
        }

	protected:

        void do_fetch( ml::frame_type_ptr &result )
        {
            scoped_lock lock( mutex_ ); 
            do_fetch( result, lock );
        }
        
		void do_fetch( ml::frame_type_ptr &result, scoped_lock& lck )
		{
			ml::input_type_ptr input = fetch_slot( 0, lck );

			if ( input && input->get_frames( ) > 0 )
			{
				int old_speed = speed_;
				int position = get_position( );
				bool changed = position != last_position_;
				bool blank_audio = false;

				if ( changed )
				{
					speed_ = position - last_position_;
					last_position_ = position;
				}

				if ( active_ == 0 )
				{
					PL_LOG( debug_level( ) + pl::level::unknown, boost::format( "requesting %d from dormant" ) % position );
					input->seek( position );
					result = input->fetch( );
					return;
				}
				else if ( !changed || speed_ != old_speed || std::abs( speed_ ) > 16 )
				{
					PL_LOG( debug_level( ) + pl::level::unknown, boost::format( "requesting %d from paused" ) % position );

					if ( changed )
					{
						PL_LOG( debug_level( ) + pl::level::unknown, boost::format( "pausing %d to %d" ) % last_position_ % position );
						pause( lck );
					}

					if ( last_frame_ && last_frame_->get_position( ) == position )
					{
						PL_LOG( debug_level( ) + pl::level::unknown, boost::format( "last frame repeat %d" ) % position );
						result = last_frame_;
					}
					else
					{
						ml::frame_type_ptr cache = cached( position, lck );
						if( !cache )
						{
							pause( lck );
							PL_LOG( debug_level( ) + pl::level::unknown, boost::format( "cache repopulate %d" ) % position );
							analyse_cache( position, speed_, lck, true );
							input->seek( position );
							result = input->fetch( );
							cache_[ result->get_position( ) ] = result;
						}
						else
						{
							PL_LOG( debug_level( ) + pl::level::unknown, boost::format( "retrieve from cache %d" ) % position );
							result = cache;
						}
					}
				}
				else
				{
					if ( !is_running( lck ) || is_paused( lck ) )
					{
						analyse_cache( position, speed_, lck, true );
						start( lck );
					}

					PL_LOG( debug_level( ) + pl::level::unknown, boost::format( "requesting %d from active %d %d" ) % position % (unsigned int)cache_.size( ) % speed_ );

					{

						// Make sure that the cache is seeded with at least one frame before use
						if ( cache_.size( ) == 0 )
						{
							cond_.notify_all( );
                            boost::system_time st = boost::get_system_time() + boost::posix_time::seconds(5);
							cond_.timed_wait( lck, st );
						}

						while( cache_.size( ) && result == 0 )
						{
							bool made_space = false;

							// Remove least likely to be used from cache
							while ( speed_ != 0 && static_cast<int>(cache_.size( )) >= prop_queue_.value< int >( ) / 2 )
							{
								std::map< int, ml::frame_type_ptr >::iterator remove = cache_.end( );
								std::map< int, ml::frame_type_ptr >::iterator min = cache_.begin( );
								std::map< int, ml::frame_type_ptr >::iterator max = -- cache_.end( );

								if ( speed_ < 0 && max->first > position )
									remove = max;
								else if ( speed_ > 0 && min->first < position - ( prop_queue_.value< int >( ) / 4 ) )
									remove = min;

								if ( remove == cache_.end( ) )
									break;

								if ( speed_ > 0 && get_position( ) <= remove->first )
									break;

								if ( speed_ < 0 && get_position( ) >= remove->first )
									break;

								if ( remove != cache_.end( ) )
								{
									PL_LOG( debug_level( ) + pl::level::unknown, boost::format( "Removing %d at %d" ) % remove->first % speed_ );
									cache_.erase( remove );
									made_space = true;
								}
								else
								{
									break;
								}
							}

							if ( made_space )
								cond_.notify_all( );

							// Determine current, min and max
							std::map< int, ml::frame_type_ptr >::iterator res = cache_.find( position );

							// If we have a match, dereference now
							if ( res != cache_.end( ) )
							{
								result = res->second;
								PL_LOG( debug_level( ) + pl::level::unknown, boost::format( "Found %d" ) % result->get_position( ) );
							}

							// Wait for the next frame arrival
							if ( result == 0 && cache_.find( position ) == cache_.end( ) )
							{
								boost::posix_time::time_duration ms;
								if( last_frame_ && speed_ == 1 && speed_ == old_speed ) 
									ms = boost::posix_time::milliseconds( int( 2000 * last_frame_->get_duration( ) ) );
								else 
									ms = boost::posix_time::milliseconds(5000);

								res = cache_.find( position );
								if ( res != cache_.end( ) )
								{
									result = res->second;
									PL_LOG( debug_level( ) + pl::level::unknown, boost::format( "Found %d" ) % result->get_position( ) );
								}
								else if ( ! cond_.timed_wait( lck, boost::get_system_time() + ms ) )
								{
									result = last_frame_;
									blank_audio = true;
									break;
								}
							}
						}

						if ( result == 0 )
							result = last_frame_;
					}
				}

				// Keep a reference to the frame in case of reuse
				last_frame_ = result;

				if( !result ) {
					result = ml::frame_type_ptr( new ml::frame_type() );
					return;
				}

				// Create a shallow copy to ensure the reference and cache are unaffected later in the graph
				result = result->shallow( );

				// Deal with audio direction
				if ( blank_audio && result->get_audio( ) )
					memset( result->get_audio( )->pointer( ), 0, result->get_audio( )->size( ) );
				else
					handle_reverse_audio( result );
			}
		}

        bool is_running( const do_lock_t&  )
        {
            
            scoped_lock lock( mutex_ );
            return is_running( lock );
        }

		bool is_running( scoped_lock& lck)
		{			
            
			return state_ & thread_running;
		}

        bool is_paused( const do_lock_t&  )
        {
            
            scoped_lock lock( mutex_ );
            return is_paused( lock );
        }

		bool is_paused( scoped_lock& lck )
		{
            
            return (state_ & thread_paused) == 0 ? false : true;
		}

		/*bool timed_wait( scoped_lock &lock, int wait )
		{
            boost::system_time st = boost::get_system_time() + boost::posix_time::milliseconds(wait);
			return cond_.timed_wait( lock, st );
		}*/

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
						audio_reversed = !audio_reversed.value< int >( );
					}
				}
				else
				{
					pl::pcos::property audio_reversed( key_audio_reversed_ );
					if ( speed_ < 0 )
						result->set_audio( ml::audio::reverse( result->get_audio( ) ) );
					result->properties( ).append( audio_reversed = speed_ < 0 ? 1 : 0 );
				}
			}
		}

        void start( const do_lock_t&  )
        {
            scoped_lock lock( mutex_ );
            start(lock);
        }

		void start( scoped_lock& lck  )
		{
			if ( is_thread_safe( lck ) )
			{
				if ( !is_running( lck ) )
				{
					state_ |= thread_running;
					reader_ = new boost::thread( start_ );
				}
				else if ( is_paused( lck ) )
				{
					state_ ^= thread_paused;
					state_ |= thread_accepted;
					state_ ^= thread_accepted;
					cond_.notify_all( );
					while( !( state_ & thread_accepted ) )
						cond_.wait( lck );
					state_ ^= thread_accepted;
				}
			}
		}

        void pause( const do_lock_t&  )
        {
            scoped_lock lock( mutex_ );
            pause(lock);
        }

		void pause( scoped_lock& lck )
		{
            
			if ( is_thread_safe( lck ) )
			{
				if ( is_running( lck ) && !is_paused( lck ) )
				{
					state_ = thread_running | thread_paused;
					cond_.notify_all( );
					while( !( state_ & thread_accepted ) )
						cond_.wait( lck );
					state_ ^= thread_accepted;
				}
			}
		}

        void stop( const do_lock_t&  )
        {
           	scoped_lock lock( mutex_ );
           	stop( lock );
        }

		void stop( scoped_lock& lck )
		{
			if ( is_thread_safe( lck ) )
			{
				if ( is_running( lck ) || is_paused( lck ) )
				{
					{
						state_ = thread_inactive;
						cond_.notify_all( );
					}

                    if( reader_ ) {
                        lck.unlock();
                        reader_->join( );
                        delete reader_;
                        reader_ = NULL;
                    }
				}
			}
		}

        int active( const do_lock_t&  )
        {
            scoped_lock lock( mutex_ );
            return active( lock );
        }

		int active( scoped_lock& lck )
		{
			return active_;
		}

		void update_active( )
		{
            scoped_lock lck(mutex_);
			if ( is_thread_safe( lck ) )
			{
				active_ = prop_active_.value< int >( );
				
				if ( active_ == 1 && !is_running( lck ) )
					start( lck );
				else if ( active_ == 0 && ( is_running( lck ) || is_paused( lck ) ) )
					stop( lck );
				else if ( active_ == 2 && is_running( lck ) )
					pause( lck );

				{
					cache_.clear( );
					last_frame_ = ml::frame_type_ptr( );
					if ( fetch_slot( 0, lck ) )
						frames_ = fetch_slot( 0, lck )->get_frames( );
				}
			}
			else
			{
				prop_active_ = 0;
				state_ = thread_inactive;
			}
		}

		ml::frame_type_ptr cached( int position, scoped_lock& lck )
		{
			return cache_.find( position ) != cache_.end( ) ? cache_[ position ] : ml::frame_type_ptr( );
		}

		int analyse_cache( int position, int speed, scoped_lock& lck, bool clean = false )
		{
			if ( clean && !cache_.empty( ) )
				PL_LOG( debug_level( ) + pl::level::unknown, boost::format( "ANTE %d: have %d to %d want %d" ) 
                    % (unsigned int)cache_.size( ) % cache_.begin( )->first % ( -- cache_.end( ) )->first % position );

			std::map< int, ml::frame_type_ptr >::iterator current = cache_.find( position );

			if ( current == cache_.end( ) )
			{
				if ( clean )
				{
					cache_.clear( );
					PL_LOG( debug_level( ) + pl::level::unknown, boost::format( "POST %d" ) % (unsigned int)cache_.size( ) );
				}
				return 0;
			}

			int count = 1;
			int next = position + speed;

			if ( speed < 0 )
			{
				if ( current != cache_.begin( ) )
				{
					do
					{
						current --;

						if ( current->first == next )
						{
							count ++;
							next = next + speed;
						}
						else if ( clean )
						{
							int p = ( ++ current )->first;
							cache_.erase( -- current );
							current = cache_.find( p );
						}
					}
					while( current != cache_.begin( ) );
				}

				std::map< int, ml::frame_type_ptr >::iterator extra = ++ cache_.find( position );
				if ( clean && extra != cache_.end( ) )
				{
					cache_.erase( extra, cache_.end( ) );
				}
			}
			else
			{
				while( ++ current != cache_.end( ) )
				{
					if ( current->first == next )
					{
						count ++;
						next = next + speed;
					}
					else if ( clean )
					{
						int p = ( -- current )->first;
						cache_.erase( ++ current );
						current = cache_.find( p );
					}
				}

				std::map< int, ml::frame_type_ptr >::iterator extra = cache_.find( position );
				if ( clean && extra != cache_.begin( ) )
				{
					next = position - speed;
					while ( -- extra != cache_.begin( ) && extra->first == next )
						next -= speed;
					if ( extra != cache_.begin( ) )
						cache_.erase( cache_.begin( ), extra );
				}
			}

			if ( clean )
				PL_LOG( debug_level( ) + pl::level::unknown, boost::format( "POST %d" ) % (unsigned int)cache_.size( ) );

			return count;
		}

		ml::input_type_ptr wait_for_info( int &position, int &speed, int &max_size, scoped_lock& lck )
		{
			ml::input_type_ptr input = fetch_slot( 0, lck );

			position = get_position( );
			speed = speed_;
			max_size = prop_queue_.value< int >( );

			bool accept_needed = !is_running( lck );
			bool refresh = !input;

			if ( !( state_ & thread_running ) )
				return ml::input_type_ptr( );

			if ( !is_paused( lck ) && ( state_ & thread_eof ) != 0 )
			{
				state_ ^= thread_eof;
				cond_.wait( lck );
				refresh = true;
			}

			if ( is_paused( lck ) )
			{
				state_ |= thread_accepted;
				cond_.notify_all( );
				accept_needed = true;
				refresh = true;
			}

			while ( is_running( lck ) && refresh )
			{
				if ( cond_.timed_wait( lck , boost::get_system_time() + boost::posix_time::seconds(1) ) )
				{
					input = fetch_slot( 0, lck );
					frames_ = input->get_frames( );
					position = get_position( );
					speed = speed_;
					max_size = prop_queue_.value< int >( );
					refresh = false;
				}
				else
				{
					input = fetch_slot( 0, lck );
					frames_ = input->get_frames( );
				}
			}

			if ( !( state_ & thread_running ) )
			{
				return ml::input_type_ptr( );
			}
			else if ( accept_needed )
			{
				state_ |= thread_accepted;
				cond_.notify_all( );
			}
			else if ( speed == 0 ) 
			{
				speed = 1;
			}

			return input;
		}

		bool cache_frame( ml::input_type_ptr input, int position, int max_size, scoped_lock& lck )
		{
			ml::frame_type_ptr frame;

			// Pick up changes in input size here
			frames_ = input->get_frames( );

			// Seek and fetch frame
			if ( cache_.size( ) > 0 )
			{
				lck.unlock( );
				input->seek( position );
				frame = input->fetch( );
				lck.lock( );
			}
			else
			{
				input->seek( position );
				frame = input->fetch( );
			}

			if ( frame )
			{
				// Store frame if available
				if ( frame && is_running( lck ) )
				{
					// Wait for space to become available
					if ( static_cast<int>(cache_.size( )) >= max_size )
					{
						while ( is_running( lck ) && !is_paused( lck ) && static_cast<int>(cache_.size( )) >= max_size )
						{
                            cond_.timed_wait( lck , boost::get_system_time() + boost::posix_time::seconds(1) );
						}
					}
		
					if ( is_running( lck ) )
					{
						PL_LOG( debug_level( ) + pl::level::unknown, boost::format( "Adding %d" ) % frame->get_position( ) );
	
						if ( frame->get_image( ) )
							frame->get_image( )->set_writable( false );
	
						if ( frame->get_alpha( ) )
							frame->get_alpha( )->set_writable( false );
	
						cache_[ frame->get_position( ) ] = frame;
						cond_.notify_all( );
					}
				}
			}

			return frame && is_running( lck );
		}

		void run( const do_lock_t&   )
		{
			int position = 0;
			int speed = 0;
			int max_size = 0;

            scoped_lock lock( mutex_ );
			while( is_running( lock ) )
			{
				// Acquire thread info
				ml::input_type_ptr input = wait_for_info( position, speed, max_size, lock );

				if ( !input ) break;

				// Check for a sub thread - disable reverse reading if we do
				bool forward = has_sub_thread_ || speed > 0 || speed <= -4;
				forward = true;

				// Vars for handling reverse play
				bool reverse = false;
				int upper = 0;

				// If we don't have the current frame, seed the cache now
				if ( analyse_cache( position, speed, lock ) == 0 )
					if ( !cache_frame( input, position, max_size, lock ) )
						break;

				// Ensure that the position is still valid
				while( is_running( lock ) && position >= 0 && position < input->get_frames( ) )
				{
					// Resync on pause condition
					if ( is_paused( lock ) )
						break;

					// Check if the requested frame is already cached
					ml::frame_type_ptr cache = cached( position, lock );

					// If it's not cached, cache it now, unless we're in reverse mode and have already reached the upper bound
        			if( !cache && ( forward || ( reverse && position < upper ) ) )
					{
						// Current position is not cached - we only come here if the required speed is positive or there's an inner thread

						// Cache the frame - break out on failure
						if ( !cache_frame( input, position, max_size, lock ) )
							break;

						// Determine the next position - if we're in reverse mode, we change the sign here
						position += !reverse ? speed : - speed;
					}
					else if ( forward )
					{
						// Current position is cached and we're travelling forward

						// Determine the number of frames available travelling from this position at this speed
						int available = analyse_cache( position, speed, lock );

						// We're in normal play mode, so seek forward by the number available
						reverse = false;
						position += available * speed;

						// Clamp the position, break when clamped position is already cached
						position = std::min< int >( std::max< int >( 0, position ), frames_ - 1 );
						if ( ( position == 0 || position == frames_ - 1 ) && cached( position, lock ) )
						{
							state_ |= thread_eof;
							break;
						}
					}
					else
					{
						// Current position is cached or previously delivered, speed is negative

						// Determine the number of frames available travelling from this position at this speed
						int available = analyse_cache( position, speed, lock );

						// We're going to fill the cache up to this position
						upper = position;

						// Attempt to fill half of the cache at a time
						position += speed * ( max_size / 2 + available - 1 );

						// We are definitely in reverse mode now
						reverse = true;

						// Special case - make sure we always align with the current speed when nearing 0
						if ( speed != 0 && position < 0 )
						{
							position = std::max< int >( position, upper % std::abs( speed ) );
							if ( cached( position, lock ) )
							{
								state_ |= thread_eof;
								break;
							}
						}

						// Pause on 0th frame if already cached
						if ( position == 0 && cached( position, lock ) )
						{
							state_ |= thread_eof;
							break;
						}

						// Break if already provided requested frame
						if ( position == upper )
						{
							state_ |= thread_eof;
							break;
						}
					}
				}
			}

			if ( is_running( lock ) )
				state_ ^= thread_running;
			active_ = 0;
			cond_.notify_all( );			
		}

	private:
		reader_thread start_;
		boost::shared_ptr< pl::pcos::observer > obs_active_;
		pl::pcos::property prop_active_;
		int active_;
		int state_;
		pl::pcos::property prop_queue_;
		pl::pcos::property prop_audio_direction_;
		int last_position_;
		mutable boost::recursive_mutex mutex_;
		boost::condition_variable_any cond_;
		int frames_;
		std::map< int, ml::frame_type_ptr > cache_;
		boost::thread *reader_;
		int speed_;
		ml::frame_type_ptr last_frame_;
		bool has_sub_thread_;
		int position_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_threader( const pl::wstring & )
{
	return ml::filter_type_ptr( new filter_threader( ) );
}

} }

#ifdef WIN32
#pragma warning(pop)
#endif
