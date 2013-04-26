// Title filter
//
// Copyright (C) 2007 Ardendo
// Released under the terms of the LGPL.
//
// #filter:title
//
// This filter provides a titling mechanism. It provides properties for the
// title, font, font size and geometry. Internally, it generates an SVG doc
// which is passed to the SVG input for conversion to a image and is then
// compositord on to the image from the connected input.

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openpluginlib/pl/pcos/isubject.hpp>
#include <openpluginlib/pl/pcos/observer.hpp>

#include <iostream>
#include <sstream>

namespace ml = olib::openmedialib::ml;
namespace pl = olib::openpluginlib;

namespace cl = olib::opencorelib;
namespace pcos = olib::openpluginlib::pcos;

namespace aml { namespace openmedialib {

static pl::pcos::key key_deferred_( pcos::key::from_string( "deferred" ) );
static pl::pcos::key key_doc_( pcos::key::from_string( "xml" ) );
static pl::pcos::key key_x_( pcos::key::from_string( "x" ) );
static pl::pcos::key key_y_( pcos::key::from_string( "y" ) );
static pl::pcos::key key_w_( pcos::key::from_string( "w" ) );
static pl::pcos::key key_h_( pcos::key::from_string( "h" ) );
static pl::pcos::key key_background_( pcos::key::from_string( "background" ) );
static pl::pcos::key key_slots_( pcos::key::from_string( "slots" ) );
static pl::pcos::key key_mode_( pl::pcos::key::from_string( "mode" ) );

class filter_title : public ml::filter_simple
{
	public:
		// Filter_type overloads
		explicit filter_title( const std::wstring & )
			: ml::filter_simple( )
			, prop_enable_( pcos::key::from_string( "enable" ) )
			, prop_title_( pcos::key::from_string( "title" ) )
			, prop_font_( pcos::key::from_string( "font" ) )
			, prop_size_( pcos::key::from_string( "size" ) )
			, prop_x_( pcos::key::from_string( "x" ) )
			, prop_y_( pcos::key::from_string( "y" ) )
			, prop_w_( pcos::key::from_string( "w" ) )
			, prop_h_( pcos::key::from_string( "h" ) )
			, prop_fg_( pcos::key::from_string( "fg" ) )
			, prop_bg_( pcos::key::from_string( "bg" ) )
			, prop_stroke_( pcos::key::from_string( "stroke" ) )
			, prop_deferred_( pcos::key::from_string( "deferred" ) )
			, prop_mode_( pcos::key::from_string( "mode" ) )
			, prop_bevel_( pcos::key::from_string( "bevel" ) )
			, prop_halign_( pcos::key::from_string( "halign" ) )
			, prop_valign_( pcos::key::from_string( "valign" ) )
			, prop_opacity_( pcos::key::from_string( "opacity" ) )
			, prop_gutter_( pcos::key::from_string( "gutter" ) )
			, pusher_bg_( )
			, pusher_fg_( )
			, svg_( )
			, compositor_( )
		{
			properties( ).append( prop_enable_ = 1 );
			properties( ).append( prop_title_ = std::wstring( L"" ) );
			properties( ).append( prop_font_ = std::wstring( L"Terminal" ) );
			properties( ).append( prop_size_ = 36 );
			properties( ).append( prop_x_ = 0.1 );
			properties( ).append( prop_y_ = 0.75 );
			properties( ).append( prop_w_ = 0.8 );
			properties( ).append( prop_h_ = 0.225 );
			properties( ).append( prop_fg_ = std::wstring( L"#FFFFFF" ) );
			properties( ).append( prop_bg_ = std::wstring( L"#000000" ) );
			properties( ).append( prop_stroke_ = std::wstring( L"#000000" ) );
			properties( ).append( prop_deferred_ = 0 );
			properties( ).append( prop_mode_ = std::wstring( L"corrected" ) );
			properties( ).append( prop_bevel_ = 0 );
			properties( ).append( prop_halign_ = 0 );
			properties( ).append( prop_valign_ = 0 );
			properties( ).append( prop_opacity_ = 1.0 );
			properties( ).append( prop_gutter_ = 5 );

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
			assert( svg_ != 0 );
			svg_->init( );
		}

		virtual ~filter_title( )
		{
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const 
		{ return prop_enable_.value< int >( ) == 1 && prop_deferred_.value< int >( ) == 0; }

		// This provides the name of the plugin (used in serialisation)
		virtual const std::wstring get_uri( ) const { return L"title"; }

		virtual const size_t slot_count( ) const { return 1; }

		virtual bool is_thread_safe( ) { return false; }

	protected:
		// The main access point to the filter
		void do_fetch( ml::frame_type_ptr &result )
		{
			result = fetch_from_slot( );

			if ( result && result->get_image( ) && prop_enable_.value< int >( ) && prop_title_.value< std::wstring >( ) != L"" )
			{
				// Ensure the inner components agrees with the deferred state
				compositor_->properties( ).get_property_with_key( key_deferred_ ) = prop_deferred_.value< int >( );

				// Determine dimensions based on our background
				int w = int( 0.5 + prop_w_.value< double >( ) * result->get_image( )->width( ) * result->get_sar_num( ) / result->get_sar_den( ) );
				int h = int( 0.5 + prop_h_.value< double >( ) * result->get_image( )->height( ) );
				w += w % 2;
				h += h % 2;

				// Pass the doc to the svg input
				svg_->properties( ).get_property_with_key( key_doc_ ) = create_doc( result, w, h );

				// Fetch image and assign geometry
				ml::frame_type_ptr fg = svg_->fetch( );
				assign( fg, key_x_, prop_x_.value< double >( ) );
				assign( fg, key_y_, prop_y_.value< double >( ) );
				assign( fg, key_w_, prop_w_.value< double >( ) );
				assign( fg, key_h_, prop_h_.value< double >( ) );

				pl::pcos::property prop( key_mode_ );
				fg->properties( ).append( prop = prop_mode_.value< std::wstring >( ) );

				if( !result->properties( ).get_property_with_key( key_background_ ).valid( ) )
				{
					pl::pcos::property background( key_background_ );
					result->properties().append( background = 2 );
					pl::pcos::property slots( key_slots_ );
					result->properties().append( slots = 1 );
				}

				// Offer to the internal compositor graph
				result->set_position( get_position( ) );
				fg->set_position( get_position( ) );
				pusher_bg_->push( result );
				pusher_fg_->push( fg );
				compositor_->seek( get_position( ) );
				result = compositor_->fetch( );

				// Ensure the position matches
				result->set_position( get_position( ) );
			}
		}

		// Assigns a value to a property
		void assign( ml::frame_type_ptr frame, pl::pcos::key name, double value )
		{
			if ( frame->properties( ).get_property_with_key( name ).valid( ) )
			{
				frame->properties( ).get_property_with_key( name ) = value;
			}
			else
			{
				pl::pcos::property prop( name );
				frame->properties( ).append( prop = value );
			}
		}

		void assign( ml::frame_type_ptr frame, pl::pcos::key name, int value )
		{
			if ( frame->properties( ).get_property_with_key( name ).valid( ) )
			{
				frame->properties( ).get_property_with_key( name ) = value;
			}
			else
			{
				pl::pcos::property prop( name );
				frame->properties( ).append( prop = value );
			}
		}

		std::wstring create_doc( ml::frame_type_ptr &result, int w, int h )
		{
			std::wstringstream stream;
			std::wstring title = prop_title_.value< std::wstring >( );
			std::wstring font = prop_font_.value< std::wstring >( );
			std::wstring fg = prop_fg_.value< std::wstring >( );
			std::wstring bg = prop_bg_.value< std::wstring >( );
			std::wstring stroke = prop_stroke_.value< std::wstring >( );
			int size = prop_size_.value< int >( );
			int bevel = prop_bevel_.value< int >( );
			int halign = prop_halign_.value< int >( );
			double opacity = prop_opacity_.value< double >( );

			int x = prop_gutter_.value< int >( );
			std::wstring anchor = L"start";

			if ( w != -1 && h != -1 )
			{
				switch( halign )
				{
					case 1:
						anchor = L"middle";
						x = w / 2;
						break;
	
					case 2:
						anchor = L"end";
						x = w - x;
						break;

					case 0:
					default:
						break;
				}
			}

			if ( title == L"@position" )
			{
				std::wstringstream ss;
				ss << result->get_position( );
				ss >> title;
			}
			else if ( title == L"@tc" )
			{
				double secs = double( result->get_position( ) ) / result->get_fps_num( ) * result->get_fps_den( );
				std::wstringstream ss;
				ss << secs;
				ss >> title;
			}

			std::vector< std::wstring > lines;
			lines.push_back( title );
			while( lines[ lines.size( ) - 1 ].find( L"\\n" ) != std::wstring::npos )
			{
				std::wstring temp = lines[ lines.size( ) - 1 ];
				lines[ lines.size( ) - 1 ] = temp.substr( 0, temp.find( L"\\n" ) );
				lines.push_back( temp.substr( temp.find( L"\\n" ) + 2 ) );
			}

			stream << "<?xml version='1.0' standalone='no'?>";
			stream << "<!DOCTYPE svg PUBLIC '-//W3C//DTD SVG 1.1//EN' 'http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd'>";
			if ( w != -1 && h != -1 )
				stream << "<svg width='" << w << "px' height='" << h << "px' version='1.1' xmlns='http://www.w3.org/2000/svg'>";
			else
				stream << "<svg width='100%' height='100%' version='1.1' xmlns='http://www.w3.org/2000/svg'>";
			if ( w != -1 && h != -1 )
				stream << "<rect x='0' y='0' width='" << w << "' height='" << h << "' style='fill:" << bg << ";opacity:" << opacity << "' rx='" << bevel << "' ry='" << bevel << "' />";
			stream << "<text x='0' y='0' text-anchor='" << anchor << "' fill='" << fg << "' stroke='" << stroke << "' style='font-size:" << size << "px;font-family:" << font << "' >";
			for ( std::vector< std::wstring >::iterator it = lines.begin( ); it != lines.end( ); ++it )
				stream << "<tspan x='" << x << "' dy='1em' text-anchor='" << anchor << "'>" << *it << "</tspan>";
			stream << "</text>";
			stream << "</svg>";

			return std::wstring( stream.str( ) );
		}

		pcos::property prop_enable_;
		pcos::property prop_title_;
		pcos::property prop_font_;
		pcos::property prop_size_;
		pcos::property prop_x_;
		pcos::property prop_y_;
		pcos::property prop_w_;
		pcos::property prop_h_;
		pcos::property prop_fg_;
		pcos::property prop_bg_;
		pcos::property prop_stroke_;
		pcos::property prop_deferred_;
		pcos::property prop_mode_;
		pcos::property prop_bevel_;
		pcos::property prop_halign_;
		pcos::property prop_valign_;
		pcos::property prop_opacity_;
		pcos::property prop_gutter_;
		ml::input_type_ptr pusher_bg_;
		ml::input_type_ptr pusher_fg_;
		ml::input_type_ptr svg_;
		ml::filter_type_ptr compositor_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_title( const std::wstring &resource )
{
	return ml::filter_type_ptr( new filter_title( resource ) );
}

} }
