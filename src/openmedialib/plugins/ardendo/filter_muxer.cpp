// Muxer filter
//
// Copyright (C) 2007 Ardendo
// Released under the terms of the LGPL.
//
// #filter:muxer
//
// Joins video and audio graphs.
//
// Example:
//
// file.mpg
// file.mp2
// filter:muxer
//
// Will display the video from file.mpg and provide the audio from file.mp2.

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"

#include <iostream>

namespace aml { namespace openmedialib {

class ML_PLUGIN_DECLSPEC filter_muxer : public ml::filter_type
{
	public:
		// Filter_type overloads
		explicit filter_muxer( )
			: ml::filter_type( )
			, prop_slot_( pcos::key::from_string( "slot" ) )
			, prop_use_longest_( pcos::key::from_string( "use_longest" ) )
		{
			properties( ).append( prop_slot_ = -1 );
			properties( ).append( prop_use_longest_ = 1 );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		// This provides the name of the plugin (used in serialisation)
		virtual const pl::wstring get_uri( ) const { return L"muxer"; }

		virtual const size_t slot_count( ) const { return 2; }

		virtual int get_frames( ) const
		{
			int frames_0 = 0;
			int frames_1 = 0;

			if ( fetch_slot( 0 ) )
				frames_0 = fetch_slot( 0 )->get_frames( );
			if ( fetch_slot( 1 ) )
				frames_1 = fetch_slot( 1 )->get_frames( );

			if ( frames_0 == 0 || frames_1 == 0 )
				return frames_0 != 0 ? frames_0 : frames_1;

			if ( prop_use_longest_.value< int >( ) )
				return frames_0 > frames_1 ? frames_0 : frames_1;

			return frames_0 < frames_1 ? frames_0 : frames_1;
		}

	protected:
		// The main access point to the filter
		void do_fetch( ml::frame_type_ptr &result )
		{
			acquire_values( );

			int slot = prop_slot_.value< int >( );

			if ( slot == -1 )
			{
				ml::input_type_ptr input_0 = fetch_slot( 0 );
				ml::input_type_ptr input_1 = fetch_slot( 1 );

				if ( input_0 && input_1 && input_0->get_frames( ) && input_1->get_frames( ) )
				{
					result = fetch_from_slot( 0, false );
					ml::frame_type_ptr overlay = fetch_from_slot( 1, false );

					if ( result && overlay && overlay->get_audio( ) )
					{
						result->set_audio( overlay->get_audio( ) );
						join_peaks( result, overlay );
					}
				}
				else if ( input_0 && input_0->get_frames( ) )
				{
					result = fetch_from_slot( 0, false );
				}
				else if ( input_1 && input_1->get_frames( ) )
				{
					result = fetch_from_slot( 1, false );
				}
			}
			else
			{
				result = fetch_from_slot( slot, false );
			}

			if ( result )
				result->set_position( get_position( ) );
		}

	private:
		pl::pcos::property prop_slot_;
		pl::pcos::property prop_use_longest_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_muxer( const pl::wstring & )
{
	return ml::filter_type_ptr( new filter_muxer( ) );
}

} }
