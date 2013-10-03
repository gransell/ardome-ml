// Copyright (C) 2010-2013 Vizrt
// Released under the LGPL.
//
// #filter:voodoo
//
// This is a variant on the fork filter which allows video and audio filters to
// be carried out on their own sub graphs.
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

filter_type_ptr create_voodoo()
{
	return filter_type_ptr( new filter_voodoo() );
}

} } } }
