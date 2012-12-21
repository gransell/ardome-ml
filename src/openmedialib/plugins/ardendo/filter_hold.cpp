// Hold filter
//
// Copyright (C) 2012 VizRT
// Released under the LGPL.
//
// #filter:hold
//
// Increases the length of the input by a number of frames by holding the
// requested frame.
//
// Example:
//
// file.mpg
// filter:hold frame=0 count=10
//
// will provide 10 extra frames by duplicating frame 0. Silent audio of the
// (more or less) correct duration will be provided during the duration.
//
// Frame numbers should be positive if the input is a growing source, but a 
// negative is used to specify a position relative to end of the input. So
// to hold the first and last frame, the following will suffice:
//
// file.mpg
// filter:hold frame=0 count=10
// filter:hold frame=-1 count=10
//
// It can be disabled by specifying a count <= 0.
//
// Notes:
//
// A clip of a growing input which is expressed with positive in/out values
// is not a growing source.
//
// Excessive use of this filter for counts which are not an exact multiple
// of the audio pattern length associated to the frame rate will cause minor
// audio sync issues (but it will take a lot of such use to make a discernable
// difference).
//
// Multiple uses of this filter have an accumalative effect, so care should be
// taken when removing or changing upstream holds since downstream holds will
// result in trying to hold an invalid/out of range frame or a different one.
// As a result, it is advised that usage is restricted to a maximum of two
// instances per clip - specfied as 0 and -1 for start and end resp.
//
// For now, out of range requests are silently ignored (I think they warrant 
// a warning, but willing to accept an enforce if considered necessary).
//
// If the source is interlaced, the resultant held frames will be progressive.

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"

#include <iostream>

namespace aml { namespace openmedialib {

class ML_PLUGIN_DECLSPEC filter_hold : public ml::filter_type
{
	public:
		filter_hold( const std::wstring & )
		: prop_frame_( pcos::key::from_string( "frame" ) )
		, prop_count_( pcos::key::from_string( "count" ) )
		, src_frames_( 0 )
		{
			properties( ).append( prop_frame_ = 0 );
			properties( ).append( prop_count_ = 25 );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		virtual int get_frames( ) const
		{
			const int count = prop_count_.value< int >( );
			return count > 0 && src_frames_ != std::numeric_limits< int >::max( ) ? src_frames_ + count : src_frames_;
		}

		virtual const std::wstring get_uri( ) const { return L"hold"; }

	protected:
		void do_fetch( ml::frame_type_ptr &result )
		{
			const int frame = prop_frame_.value< int >( );
			const int count = prop_count_.value< int >( );
			const int position = get_position( );

			ml::input_type_ptr input = fetch_slot( );
			ARENFORCE_MSG( input, "No input connected to hold filter" );

			const int src_frames = input->get_frames( );
			const int hold_frame = frame < 0 ? src_frames + frame : frame;
			const bool active = hold_frame >= 0 && hold_frame < input->get_frames( ) && count > 0;

			if ( active && position >= hold_frame && position < hold_frame + count )
			{
				input->seek( hold_frame );
				result = input->fetch( );
				ARENFORCE_MSG( result, "Unable to obtain the hold frame %d for %d" )( hold_frame )( position );
				result = result->shallow( );
				result->set_position( position );
				if ( result->has_audio( ) )
					silence_audio( result, position - frame );
				if ( result->has_image( ) && result->get_image( )->field_order( ) != il::progressive )
					result->set_image( il::deinterlace( il::conform( result->get_image( ), il::writable ) ) );
			}
			else if ( active && position >= hold_frame + count )
			{
				input->seek( position - count );
				result = input->fetch( );
				ARENFORCE_MSG( result, "Unable to obtain a frame for %d" )( position );
				result = result->shallow( );
				result->set_position( position );
			}
			else
			{
				input->seek( position );
				result = input->fetch( );
			}
		}

		void sync_frames( )
		{
			src_frames_ = fetch_slot( ) ? fetch_slot( )->get_frames( ) : 0;
		}

	private:
		void silence_audio( ml::frame_type_ptr &result, int offset )
		{
			ml::audio_type_ptr audio = result->get_audio( );
			int samples = ml::audio::samples_for_frame( offset, audio->frequency( ), result->get_fps_num( ), result->get_fps_den( ) );
			audio = ml::audio::allocate( audio, -1, -1, samples );
			result->set_audio( audio );
		}

		pl::pcos::property prop_frame_;
		pl::pcos::property prop_count_;
		int src_frames_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_hold( const std::wstring &resource )
{
	return ml::filter_type_ptr( new filter_hold( resource ) );
}

} }
