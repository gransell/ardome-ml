// Copyright (C) 2010-2013 Vizrt
// Released under the LGPL.
//
// #filter:fork
//
// This provides a threadsafe variant of the filter:tee.
//

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/keys.hpp>
#include <opencorelib/cl/enforce_defines.hpp>
#include <opencorelib/cl/log_defines.hpp>

namespace pl = olib::openpluginlib;
namespace ml = olib::openmedialib::ml;
namespace il = olib::openimagelib::il;
namespace cl = olib::opencorelib;

namespace olib { namespace openmedialib { namespace ml { namespace distributor {

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

filter_type_ptr create_fork()
{
	return filter_type_ptr( new filter_fork() );
}

} } } }
