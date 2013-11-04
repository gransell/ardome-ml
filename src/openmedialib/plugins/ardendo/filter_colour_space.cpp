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
#include <openmedialib/ml/image/rescale_object.hpp>
#include <openmedialib/ml/types.hpp>

namespace aml { namespace openmedialib {

class ML_PLUGIN_DECLSPEC filter_colour_space : public ml::filter_simple
{
	public:
		// Filter_type overloads
		explicit filter_colour_space( const std::wstring & )
			: ml::filter_simple()
			, prop_pf_( pcos::key::from_string( "pf" ) )
			, ro_ ( ml::rescale_object_ptr( new ml::image::rescale_object( ) ) )
		{
			properties( ).append( prop_pf_ = olib::t_string( _CT("r8g8b8a8") ) );
			
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
			result = frame_convert( ro_, result, prop_pf_.value< olib::t_string >( ).c_str( ) );
		}

		pcos::property prop_pf_;

	private:
		ml::rescale_object_ptr ro_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_colour_space( const std::wstring &resource )
{
	return ml::filter_type_ptr( new filter_colour_space( resource ) );
}

} }
