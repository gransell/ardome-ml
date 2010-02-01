// Offset filter
//
// Copyright (C) 2007 Ardendo
// Released under the terms of the LGPL.
//
// #filter:offset
//
// This filter provides a mechanism to allow clips to offset their start time.
// Although it seems a rather simplistic filter, it is a critical one.
//
// For example, to have a composited video come in after 250 frames:
//
// colour:
// video1.mpg 
// video2.mpg 
// filter:offset in=250 
// filter:compositor slots=3
//
// Assuming video2 is shorter than video1, you could conceptualise this as a 
// track arrangement like:
//
// T0: |---------background----------|
// T1: |----------video1-------------|
// T2: |     |----video2----|
//
// Note that during the first 250 frames and after the end of video2, T2 
// provides frames which have a null image - the composite will simply show T1 
// and the geometry is ignored since it dictates how T2 is rendered on T1. 
// During the active part of video2 the composite happens as expected.
//
// If you want to clip video2 at the native frame rate, change frame rate and 
// animate the compositing geometry, you would need to specify the order as 
// follows:
//
// <video1> 
// <video2> 
// filter:clip <points> 
// filter:frame_rate <target fps>
// filter:lerp <geometry> 
// filter:offset in=250 
// filter:composite <geometry>
//
// If the intention is to provide transitions between multiple successive clips
// over two tracks like:
//
// T1: |----------video1-------------|        |-------video3--------|
// T2: |                          |----video2----|
//
// then the same logic applies, but you need to build the graph in track order
// and you need a background to composite on to.
//
// Annotating the tracks in points as follows:
//
// T0: |---------------------------background-----------------------|
// T1: |----------video1-------------|        |-------video3--------|
//     A                                      B
// T2: |                          |----video2----|
//                                C
//
// We end up with this rather wonderfully simple graph:
//
// background
// <video1> <clip/frame_rate/animate> <offset A> filter:composite <geometry>
// <video3> <clip/frame_rate/animate> <offset B> filter:composite <geometry>
// <video2> <clip/frame_rate/animate> <offset C> filter:composite <geometry>
//
// In other words, T2 is composited *after* T1. In this way, the z ordering of 
// the rendering and the track are the same. Transitions and overlays (or even 
// overlays with transitions) are all handled identically.
//
// Further, disabling tracks becomes a trivial operation (simply don't include
// the track contents in the graph build).
//
// As a justification for track order versus time order, consider the case
// where an overlay spans the entirety of the upper tracks:
//
// T0: |-------------------------background-------------------------|
// T1: |----------video1-------------|        |-------video3--------|
// T2: |                          |----video2----|
// T3: |--------------------------overlay---------------------------|
//
// If time order were used, overlay would be obscured by video2 and video3
// which is quite clearly not the intention.
//
// There is a discrepancy introduced though - a transition between T1/T2 and 
// T2/T1 is not the same since the lower track is always composited on top of 
// the upper track. For most transitions, this isn't a problem - mixes, wipes, 
// slides have an identical output providing the transition on the out going is 
// complementary to the transition on the incoming.
//
// My belief is that the track arrangement is wrong for this type of operation 
// and transitions between successive clips should always be handled on a 
// single track like:
//
// T0: |-------------------------background-------------------------|
// T1: |----------video1----------:--|-video2-:--|----video3--------|
// T2: |--------------------------overlay---------------------------|
//
// where the : indicates the start of an overlap between successive clips. This 
// is simpler in a number of ways - lower tracks are reserved purely for 
// overlays and dubbing/mixing purposes, moving clips around on the upper track 
// becomes easier (esp. doing so in a manner that preserves the transitions) 
// and there's simply no amibiguity/variance involved in the rendering process 
// (since incoming is always rendered after outgoing). 
//
// Note that the conform is needed to ensure that both images and audio are 
// eventually provided, however, in the case of a general composite onto a fixed
// background, the conform is not required.
//
// To work around some issues in the current frame_rate and resampler filters, 
// audio is always provided as the same type as the input slot (if the input slot
// is actually providing audio anyway).

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"

namespace aml { namespace openmedialib {

class ML_PLUGIN_DECLSPEC filter_offset : public ml::filter_type
{
	public:
		// Filter_type overloads
		explicit filter_offset( const pl::wstring & )
			: ml::filter_type( )
			, prop_in_( pcos::key::from_string( "in" ) )
			, prop_pad_audio_( pcos::key::from_string( "pad_audio" ) )
			, prop_fps_num_( pcos::key::from_string( "fps_num" ) )
			, prop_fps_den_( pcos::key::from_string( "fps_den" ) )
			, prop_frequency_( pcos::key::from_string( "frequency" ) )
			, prop_channels_( pcos::key::from_string( "channels" ) )
			, obs_in_changed_( new fn_observer< filter_offset >( const_cast< filter_offset * >( this ), &filter_offset::in_changed ) )
			, src_fps_num_( 25 )
			, src_fps_den_( 1 )
			, src_frequency_( 48000 )
			, src_channels_( 2 )
			, position_( 0 )
		{
			properties( ).append( prop_in_ = 0 );
			properties( ).append( prop_pad_audio_ = 0 );
			properties( ).append( prop_fps_num_ = 0 );
			properties( ).append( prop_fps_den_ = 0 );
			properties( ).append( prop_frequency_ = 0 );
			properties( ).append( prop_channels_ = 0 );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		// This provides the name of the plugin (used in serialisation)
		virtual const pl::wstring get_uri( ) const { return L"offset"; }

		// Number of frames provided by this filter is in + frames in connected input
		virtual int get_frames( ) const
		{
			int result = prop_in_.value< int >( );
			ml::input_type_ptr input = fetch_slot( );
			if ( input )
				result += input->get_frames( );
			return result;
		}

		// We need to obtain the frame rate and audio properties on a connect - these can be provided
		// via the properties or via a fetch on the frame - the property usage is prefered since it allows
		// lazy evaluation of inputs
		virtual void on_slot_change( ml::input_type_ptr input, int )
		{
			if ( input )
			{
				if ( prop_fps_num_.value< int >( ) && prop_fps_den_.value< int >( ) )
				{
					src_fps_num_ = prop_fps_num_.value< int >( );
					src_fps_den_ = prop_fps_den_.value< int >( );
					src_frequency_ = prop_frequency_.value< int >( );
					src_channels_ = prop_channels_.value< int >( );
					prop_pad_audio_ = src_frequency_ && src_channels_;
				}
				else
				{
					ml::frame_type_ptr frame = input->fetch( );
					if ( frame )
					{
						src_fps_num_ = frame->get_fps_num( );
						src_fps_den_ = frame->get_fps_den( );
						if ( frame->get_audio( ) )
						{
							src_frequency_ = frame->get_audio( )->frequency( );
							src_channels_ = frame->get_audio( )->channels( );
						}
						else
						{
							src_frequency_ = 0;
						}
						seek( 0 );
					}
				}
			}
		}
		
		void in_changed( )
		{
			// Walk the graph to update any filters that depend on the offset
			update_offset_properties( fetch_slot( ), prop_in_.value< int >( ) );
		}

	protected:
		// The main access point to the filter
		void do_fetch( ml::frame_type_ptr &result )
		{
			// Acquire values
			acquire_values( );

			// If we're between 0 and the in point, return a valid but empty frame
			// otherwise fetch from the connected input seeking to the position - in
			if ( get_position( ) < prop_in_.value< int >( ) || get_position( ) >= get_frames( ) )
			{
				result = ml::frame_type_ptr( new ml::frame_type( ) );
			}
			else
			{
				ml::input_type_ptr input = fetch_slot( );
				if ( input )
				{
					input->seek( get_position( ) - prop_in_.value< int >( ) );
					result = input->fetch( );
					assign_frame_props( result );
				}
			}

			// If we definitely have a frame, set its position
			if ( result )
			{
				result->set_position( get_position( ) );
				result->set_fps( src_fps_num_, src_fps_den_ );

				// Generate audio if required
				if ( prop_pad_audio_.value< int >( ) && src_frequency_ && !result->get_audio( ) )
				{
					int samples = ml::audio::samples_for_frame( get_position( ), src_frequency_, src_fps_num_, src_fps_den_ );
					ml::audio::pcm16_ptr aud = ml::audio::pcm16_ptr( new ml::audio::pcm16( src_frequency_, src_channels_, samples ) );
					result->set_audio( aud );
				}
			}
		}
		
		void update_offset_properties( const ml::input_type_ptr& to_update, int new_offset )
		{
			// Break recursion if we hit one more offset filter
			if( to_update->get_uri( ) == L"offset" ) return;
			
			// Check if the input has a offset prop and if so update
			pcos::property p = to_update->properties( ).get_property_with_string( "offset" );
			if( p.valid( ) ) p = new_offset;
			
			// Walk all children
			for( int i = 0; i < to_update->slot_count( ); ++i )
			{
				update_offset_properties( to_update->fetch_slot( i ), new_offset );
			}
		}

		pcos::property prop_in_;
		pcos::property prop_pad_audio_;
		pcos::property prop_fps_num_;
		pcos::property prop_fps_den_;
		pcos::property prop_frequency_;
		pcos::property prop_channels_;
		boost::shared_ptr< pl::pcos::observer > obs_in_changed_;
		int src_fps_num_;
		int src_fps_den_;
		int src_frequency_;
		int src_channels_;
		int position_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_offset( const pl::wstring &resource )
{
	return ml::filter_type_ptr( new filter_offset( resource ) );
}

} }
