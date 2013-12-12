// Copyright (C) 2010-2013 Vizrt
// Released under the LGPL.
//
// #filter:lock
//
// This filter is used to protect non-threadsafe inputs so that they can be 
// safely accessed by multiple concurrent threads. See filter:distributor for
// more details.
//

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/keys.hpp>
#include <opencorelib/cl/enforce_defines.hpp>
#include <opencorelib/cl/log_defines.hpp>

namespace pl = olib::openpluginlib;
namespace ml = olib::openmedialib::ml;
namespace cl = olib::opencorelib;

namespace olib { namespace openmedialib { namespace ml { namespace distributor {

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
				ml::image_type_ptr image = frame->get_image( );
				frame->set_image( image ? ml::image_type_ptr( static_cast<ml::image::image*>( image->clone( ml::image::writable ) ) ) : image );
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

filter_type_ptr create_lock()
{
	return filter_type_ptr( new filter_lock() );
}

} } } }
