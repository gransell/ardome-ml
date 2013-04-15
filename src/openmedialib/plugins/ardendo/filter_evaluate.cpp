// Evaluate filter
//
// Copyright (C) 2013 Vizrt
// Released under the LGPL.
//
// #fliter:evaluate
//
// This filter is used to force evaluation of a component (component=audio, component=stream, component=image). 
// It calls the corresponding get_<component> function. Useful for testing/debugging purposes. For example, to force evaluation of an image from a stream.

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"

namespace aml { namespace openmedialib {

class ML_PLUGIN_DECLSPEC filter_evaluate : public ml::filter_simple
{
	public:
		// Filter_type overloads
		explicit filter_evaluate( const std::wstring & )
			: ml::filter_simple( )
			, prop_component_( pcos::key::from_string( "component" ) ) 
		{
			properties( ).append( prop_component_ = std::wstring( L"" ) );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		// This provides the name of the plugin (used in serialisation)
		virtual const std::wstring get_uri( ) const { return L"evaluate"; }

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
				result->get_image();
			else if (component == L"audio")
				result->get_audio();
			else if (component == L"stream")
				result->get_stream();
			else
				ARENFORCE_MSG( 0, "Invalid value for component: %1%. Valid values: audio, image, stream." )( component );
				
		}

	protected:
		pcos::property prop_component_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_evaluate( const std::wstring &resource )
{
	return ml::filter_type_ptr( new filter_evaluate( resource ) );
}

} }
