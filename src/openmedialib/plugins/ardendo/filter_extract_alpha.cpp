// Extract Alpha Filter
//
// Copyright (C) 2013 Vizrt
// Released under the terms of the LGPL.
//
// #filter:extract_alpha
//
// Replaces the image with an alpha mask or extract channel/plane as approriate. 
// Only useful for testing.

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"

namespace aml { namespace openmedialib {

class ML_PLUGIN_DECLSPEC filter_extract_alpha : public ml::filter_simple
{
	public:
		// Filter_type overloads
		explicit filter_extract_alpha( const std::wstring & )
		: prop_enable_( pcos::key::from_string( "enable" ) )
		{
			properties( ).append( prop_enable_ = 1 );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return true; }

		// This provides the name of the plugin (used in serialisation)
		virtual const std::wstring get_uri( ) const { return L"extract_alpha"; }

	protected:
		// The main access point to the filter
		void do_fetch( ml::frame_type_ptr &result )
		{
			// Frame to return
			result = fetch_from_slot( );

			// Handle the frame with image case
			if ( prop_enable_.value< int >( ) && result && result->has_image( ) )
			{
				result = result->shallow( );

				if ( !result->get_alpha( ) ) 
				{
					ml::image_type_ptr alpha = ml::image::extract_alpha( result->get_image( ) );
					if ( alpha )
						result->set_image( alpha );
				}
				else
				{
					result->set_image( result->get_alpha( ) );
					result->set_alpha( ml::image_type_ptr( ) );
				}
			}
		}

	protected:
		pcos::property prop_enable_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_extract_alpha( const std::wstring &resource )
{
	return ml::filter_type_ptr( new filter_extract_alpha( resource ) );
}

} }
