// An SVG reader
//
// Copyright (C) 2007 Ardendo
// Released under the LGPL.
//
// #input:svg:
//
// An SVG input based on librsvg.

#include <opencorelib/cl/enforce_defines.hpp>
#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openpluginlib/pl/utf8_utils.hpp>
#include <openpluginlib/pl/pcos/isubject.hpp>
#include <openpluginlib/pl/pcos/observer.hpp>

#include <iostream>
#include <boost/thread.hpp>

#include <librsvg/rsvg.h>

namespace ml = olib::openmedialib::ml;
namespace pl = olib::openpluginlib;
namespace il = olib::openimagelib::il;
namespace cl = olib::opencorelib;
namespace pcos = olib::openpluginlib::pcos;

namespace aml { namespace openmedialib { 

class ML_PLUGIN_DECLSPEC filter_librsvg : public ml::filter_type
{
	public:
		filter_librsvg( const pl::wstring &resource ) 
			: ml::filter_type( )
			, prop_file_( pcos::key::from_string( "file" ) )
			, prop_xml_( pcos::key::from_string( "xml" ) )
			, prop_duration_( pcos::key::from_string( "duration" ) )
			, prop_deferred_( pcos::key::from_string( "deferred" ) )
			, prop_stretch_( pcos::key::from_string( "stretch" ) )
		{
			properties( ).append( prop_file_ = pl::wstring(L"") );
			properties( ).append( prop_xml_ = pl::wstring(L"") );
			properties( ).append( prop_duration_ = -1 );
			properties( ).append( prop_deferred_ = 0 );
			properties( ).append( prop_stretch_ = 0 );

			pusher_bg_ = ml::create_input( L"pusher:" );
			pusher_fg_ = ml::create_input( L"pusher:" );
			compositor_ = ml::create_filter( L"compositor" );

			assert( pusher_bg_ != 0 );
			assert( pusher_fg_ != 0 );
			assert( compositor_ != 0 );

			pusher_bg_->init( );
			pusher_fg_->init( );
			compositor_->init( );

			compositor_->connect( pusher_bg_, 0 );
			compositor_->connect( pusher_fg_, 1 );

			svg_ = ml::create_input( L"svg:" );
			ARENFORCE_MSG( svg_ != 0, "Failed to create backing input for SVG filter" );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return true; }

		// Basic information
		virtual const pl::wstring get_uri( ) const { return L"svg"; }

		virtual const size_t slot_count( ) const { return 1; }

		void do_fetch( ml::frame_type_ptr &result )
		{
			acquire_values( );

			result = fetch_from_slot( );
			
			int dur = prop_duration_.value<int>( );
			if ( dur != -1 && get_position( ) >= dur )
			{
				return;
			}

			if ( result && result->get_image( ) )
			{
				svg_->properties( ).get_property_with_string( "fps_num" ) = result->get_fps_num( );
				svg_->properties( ).get_property_with_string( "fps_den" ) = result->get_fps_den( );

				// Ensure the inner compositor agrees with the deferred state
				compositor_->properties( ).get_property_with_string( "deferred" ) = prop_deferred_.value< int >( );

				// Determine dimensions based on our background
				svg_->properties( ).get_property_with_string( "render_width" ) = result->get_image( )->width( );
				svg_->properties( ).get_property_with_string( "render_height" ) = result->get_image( )->height( );
				svg_->properties( ).get_property_with_string( "stretch" ) = prop_stretch_.value< int >();

				int sar_num = 0, sar_den = 0;
				result->get_sar( sar_num, sar_den );
				svg_->properties( ).get_property_with_string( "render_sar_num" ) = sar_num;
				svg_->properties( ).get_property_with_string( "render_sar_den" ) = sar_den;

				// Pass the SVG XML string/filename to the SVG input
				svg_->properties( ).get_property_with_string( "xml" ) = prop_xml_.value< pl::wstring >( );
				svg_->properties( ).get_property_with_string( "resource" ) = prop_file_.value< pl::wstring >();


				// Fetch image
				ml::frame_type_ptr fg = svg_->fetch( );

				if( !result->properties( ).get_property_with_string( "background" ).valid( ) )
				{
					pl::pcos::property background( pl::pcos::key::from_string( "background" ) );
					result->properties().append( background = 2 );
					pl::pcos::property slots( pl::pcos::key::from_string( "slots" ) );
					result->properties().append( slots = 1 );
				}

				// Offer to the internal compositor graph
				pusher_bg_->push( result );
				pusher_fg_->push( fg );
				compositor_->seek( get_position() );
				result = compositor_->fetch( );

				// Ensure the position matches
				result->set_position( get_position( ) );
			}
		}

	protected:

		pcos::property prop_file_;
		pcos::property prop_xml_;
		pcos::property prop_duration_;
		pcos::property prop_deferred_;
		pcos::property prop_stretch_;
		ml::input_type_ptr pusher_bg_;
		ml::input_type_ptr pusher_fg_;
		ml::input_type_ptr svg_;
		ml::filter_type_ptr compositor_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_filter_librsvg( const pl::wstring &resource )
{
	return ml::filter_type_ptr( new filter_librsvg( resource ) );
}

} }

