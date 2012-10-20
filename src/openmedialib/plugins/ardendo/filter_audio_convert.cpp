// Audio conversion filter
//
// Copyright (C) 2009 Ardendo
// Released under the LGPL.
//
// #filter:audio_convert
//
// This filter provides a very basic mechanism which attempts to ensure that
// the audio in the fetched frame is in a specific format.
//
// Example:
//
// file.mp2
// filter:audio_convert af=float
//
// Converts the audio from file.mp2 to a floating point representation.
//
// Note, this filter is rarely useful other than for testing.

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"

namespace aml { namespace openmedialib {

class ML_PLUGIN_DECLSPEC filter_audio_convert : public ml::filter_simple
{
	public:
		// Filter_type overloads
		explicit filter_audio_convert( )
			: ml::filter_simple( )
			, prop_af_( pcos::key::from_string( "af" ) )
		{
			properties( ).append( prop_af_ = std::wstring( ml::audio::FORMAT_FLOAT ) );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		// This provides the name of the plugin (used in serialisation)
		virtual const std::wstring get_uri( ) const { return L"audio_convert"; }

	protected:
		// The main access point to the filter
		void do_fetch( ml::frame_type_ptr &result )
		{
			// Frame to return
			result = fetch_from_slot( );

			// Handle the frame with image case
			if ( result && result->get_audio( ) )
				change_audio( result );
		}

	protected:

		// Change the colour space to the requested
		void change_audio( ml::frame_type_ptr &result )
		{
			ml::audio_type_ptr audio = ml::audio::coerce( prop_af_.value< std::wstring >( ), result->get_audio( ) );
			if ( audio )
				result->set_audio( audio );
		}

		pcos::property prop_af_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_audio_convert( const std::wstring &resource )
{
	return ml::filter_type_ptr( new filter_audio_convert( ) );
}

} }
