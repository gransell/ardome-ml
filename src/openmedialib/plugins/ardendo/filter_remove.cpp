// Remove filter
//
// Copyright (C) 2013 Vizrt
// Released under the LGPL.
//
// #fliter:remove
//
// This filter is used to remove a component (component=audio, component=stream, component=image) from a frame. 
// Useful for testing/debugging purposes.

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"

namespace aml { namespace openmedialib {

class ML_PLUGIN_DECLSPEC filter_remove : public ml::filter_simple
{
	public:
		// Filter_type overloads
		explicit filter_remove( const std::wstring & )
			: ml::filter_simple( )
			, prop_component_( pcos::key::from_string( "component" ) ) 
		{
			properties( ).append( prop_component_ = std::wstring( L"" ) );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		// This provides the name of the plugin (used in serialisation)
		virtual const std::wstring get_uri( ) const { return L"remove"; }

	protected:
		// The main access point to the filter
		void do_fetch( ml::frame_type_ptr &result )
		{
			// Acquire values
			acquire_values( );

			// Frame to return
			result = fetch_from_slot( );
			
			std::wstring component =  prop_component_.value< std::wstring >();
			if (component == L"image")
				result->set_image( olib::openimagelib::il::image_type_ptr() );
			else if (component == L"audio")
				result->set_audio( olib::openmedialib::ml::audio_type_ptr() );
			else if (component == L"stream")
				result->set_stream( olib::openmedialib::ml::stream_type_ptr() );
			else
				ARENFORCE_MSG( 0, "Invalid value for component: %1%. Valid values: audio, image, stream." )( component );
				
		}

	protected:
		pcos::property prop_component_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_remove( const std::wstring &resource )
{
	return ml::filter_type_ptr( new filter_remove( resource ) );
}

} }
