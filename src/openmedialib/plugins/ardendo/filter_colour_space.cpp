// Colour space filter 
//
// Copyright (C) 2007 Ardendo
// Released under the LGPL.
//
// #filter:colour_space
//
// This filter provides a very basic mechanism which attempts to ensure that
// the image in the fetched frame is in a specific colour space.
//
// Example:
//
// file.mpg
// filter:colour_space pf=yuv422p
//
// Will convert the incoming images from file.mpg to a yuv422p picture format.

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"

namespace aml { namespace openmedialib {

class ML_PLUGIN_DECLSPEC filter_colour_space : public ml::filter_simple
{
	public:
		// Filter_type overloads
		explicit filter_colour_space( const std::wstring & )
			: ml::filter_simple()
			, prop_pf_( pcos::key::from_string( "pf" ) )
		{
			properties( ).append( prop_pf_ = olib::t_string( "r8g8b8a8" ) );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return true; }

		// This provides the name of the plugin (used in serialisation)
		virtual const std::wstring get_uri( ) const { return L"colour_space"; }

	protected:
		// The main access point to the filter
		void do_fetch( ml::frame_type_ptr &result )
		{
			// Frame to return
			result = fetch_from_slot( );

			// Handle the frame with image case
			if ( result && result->get_image( ) && ( olib::t_string( result->get_image( )->pf( ) ) != prop_pf_.value< olib::t_string >( ) ) )
				change_colour_space( result );
		}

	protected:

		// Change the colour space to the requested
		void change_colour_space( ml::frame_type_ptr & result )
		{
			// Take a back up just in case
			ml::image_type_ptr image = result->get_image( );

			// Try a straight convert
			result = frame_convert( result, prop_pf_.value< olib::t_string >( ).c_str( ) );

			// If that failed, the convert is missing, so go through a proxy format first
			if ( !result->get_image( ) )
			{
				// Inefficient back up plan
				ml::image_type_ptr alternative = ml::image::convert( image, "b8g8r8a8" );
				alternative = ml::image::convert( alternative, olib::t_string( prop_pf_.value< olib::t_string >( ).c_str( ) ) );

				// If we now have an image, assign to the frame, otherwise leave the original 
				// colourspace in place
				if ( alternative )
					result->set_image( alternative );
				else
					result->set_image( image );
			}
		}

		pcos::property prop_pf_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_colour_space( const std::wstring &resource )
{
	return ml::filter_type_ptr( new filter_colour_space( resource ) );
}

} }
