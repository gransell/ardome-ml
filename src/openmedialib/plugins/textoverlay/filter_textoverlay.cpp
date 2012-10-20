// Text overlay filter
//
// Copyright (C) 2007 Ardendo
// Released under the terms of the LGPL.
//
// #filter:textoverlay
//
// This filter provides a overlaying arbitrary text on top of video. It
// provides properties for the title, font, font size, font options, color for
// fill and stroke, geometry and optionally a background box. Internally, it
// renders on a cairo surface converted to an image put on a frame and
// composited internally (not deferred).
//
// The textoverlay filter can overlay text with lots of options for font,
// size, etc. It doesn't yet support deferred frames, but when printing the
// same text with the same options (technically as long as the "dirty"
// property is still false) the overlayed image will be cached and not redrawn
// which although being fast, takes its time.
//
// It has the following properties:
// type:
// 	"tc" for timecodes or "text" [default] for custom text
// text:
//	Custom text to draw (only applicable when type=text)
// color:
//	The text color on the format #RRGGBB(AA)
// stroke_width:
//	The width of the stroke if the text should have an outer stroke. If in
//	percent, it's relative to the font height (around 3-5% is good).
// stroke_color:
//	The color of the outer stroke on the format #RRGGBB(AA)
// font_face:
//	The font (such as "Sans")
// font_size:
//	The font size. If in percent, it's relative to the height of the frame
//	which makes it easy to specify a size that is resolution agnostic.
// font_slant:
//	"normal" or "italic" or "oblique"
// font_weight:
//	"normal" or "bold"
// position:
//	As "x,y" where any of x and/or y can be either in pixels or in percent
//	which is relative to the frame width and height respectively.
// origin:
//	The origin from where the position is measured:
//	top_left, top_right, bottom_left or bottom_right
//	So, origin "top_left" at position "0,0" equals origin "bottom_right" at
//	position "100%,100%"
// alignment:
//	The direction at which the text is drawn.
//	right_up, right_middle, right_down, left_up, left_middle, left_down,
//	center_up, center_middle or center_down.
//	right_up draws the text to the right of the position and upward, i.e.
//	the position defines the bottom left point of the text.
//	left_down draws to the left and down, i.e. the position defines the
//	upper right point of the text.
//	This doesn't affect the text direction per se, just where to place the
//	text. In other words, text is always LTR.
// box_color:
//	The color of the background box on the format #RRGGBB(AA). Defaults to
//	a color with alpha==0 which means invisible. No box is then rendered.
// box_padding:
//	The amount of space surrounding the text that will be filled. Either
//	in pixels or in percentage of the font height. So, "20%" means that
//	each side of the text will be padded (and painted) by a fifth of the
//	font height.
//

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openpluginlib/pl/pcos/isubject.hpp>
#include <openpluginlib/pl/pcos/observer.hpp>
#include <opencorelib/cl/enforce_defines.hpp>
#include <opencorelib/cl/time_code.hpp>
#include <opencorelib/cl/media_time.hpp>
#include <opencorelib/cl/media_definitions.hpp>

#include <iostream>
#include <sstream>

#include <external/cairo/cairo.hpp>

namespace ml = olib::openmedialib::ml;
namespace pl = olib::openpluginlib;
namespace il = olib::openimagelib::il;
namespace cl = olib::opencorelib;
namespace pcos = olib::openpluginlib::pcos;
namespace ext = aml::external;
namespace cairo = ext::cairo;

namespace aml { namespace openmedialib {

class filter_textoverlay : public ml::filter_simple
{
public:
	// Filter_type overloads
	explicit filter_textoverlay( const std::wstring & );
	virtual ~filter_textoverlay( );

	DEFINE_PROPERTY( bool, dirty )
	DEFINE_PROPERTY( std::string, type )
	DEFINE_PROPERTY( std::string, text )
	DEFINE_PROPERTY( std::string, color )
	DEFINE_PROPERTY( std::string, stroke_width )
	DEFINE_PROPERTY( std::string, stroke_color )
	DEFINE_PROPERTY( std::string, font_face )
	DEFINE_PROPERTY( std::string, font_size )
	DEFINE_PROPERTY( std::string, font_slant )
	DEFINE_PROPERTY( std::string, font_weight )
	DEFINE_PROPERTY_AS( std::string, position, textpos )
	DEFINE_PROPERTY( std::string, origin )
	DEFINE_PROPERTY( std::string, alignment )
	DEFINE_PROPERTY( std::string, box_color )
	DEFINE_PROPERTY( std::string, box_padding )

	DEFINE_PROPERTY_TYPED_AS( bool, int, deferred, deferred )

	// Indicates if the input will enforce a packet decode
	virtual bool requires_image( ) const { return true; } // No deferred support yet

	// This provides the name of the plugin (used in serialisation)
	virtual const std::wstring get_uri( ) const { return L"subtitle"; }

	virtual const size_t slot_count( ) const { return 1; }

	virtual bool is_thread_safe( ) { return false; }

protected:

	// Translates dynamic sizes (in percent) based on pixel length
	double calculate_size( double base, const std::string& sz );

	// Translates dynamic sizes (in percent) based on frame size
	double calculate_size( ml::frame_type_ptr& frame, bool vertical,
	                       const std::string& sz );

	// Translates dynamic sizes (in percent) based on frame size
	cairo::rect calculate_sizes( ml::frame_type_ptr& frame,
	                             const std::string& pos );

	std::string text_to_draw( ml::frame_type_ptr& bg_frame );
	std::string text_for_extent( ml::frame_type_ptr& bg_frame );

	void draw_text( ml::frame_type_ptr& bg_frame );

	il::image_type_ptr redraw( ml::frame_type_ptr& bg_frame );

	// The main access point to the filter
	void do_fetch( ml::frame_type_ptr& frame );

	ml::input_type_ptr pusher_bg_;
	ml::input_type_ptr pusher_fg_;
	ml::filter_type_ptr compositor_;

	cairo::surface_ptr cairo_surface_;
	cairo::context_ptr cairo_context_;
	ml::frame_type_ptr overlay_;
};


filter_textoverlay::filter_textoverlay( const std::wstring & )
: ml::filter_simple( )
, pusher_bg_( )
, pusher_fg_( )
, compositor_( ) {
	// Initialize properties
	set_dirty( true );
	set_type( "text" );
	set_text( "" );
	set_color( "#FFFFFFFF" );
	set_stroke_width( "0%" );
	set_stroke_color( "#00000000" );
	set_font_face( "Sans" );
	set_font_size( "4%" );
	set_font_slant( "normal" );
	set_font_weight( "normal" );
	set_textpos( "50%,50%" );
	set_origin( "top_left" );
	set_alignment( "center_middle" );
	set_box_color( "#00000000" );
	set_box_padding( "20%" );

	set_deferred( false );


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
}

filter_textoverlay::~filter_textoverlay( ) {
}

double filter_textoverlay::calculate_size( double base, const std::string& sz )
{
	if ( sz.empty( ) )
		return 0;

	if ( sz[ sz.size( ) - 1 ] == '%' ) {
		// In percent
		std::string s = sz.substr( 0, sz.size( ) - 1 );

		return atof( s.c_str( ) ) * base / 100.;
	} else {
		// In fixed units
		return atof( sz.c_str( ) );
	}
}

// Translates dynamic sizes (in percent) based on frame size
double filter_textoverlay::calculate_size( ml::frame_type_ptr& frame,
                                           bool vertical,
                                           const std::string& sz )
{
	int base = vertical ? frame->height( ) : frame->width( );
	return calculate_size( (double)base, sz );
}

// Translates dynamic sizes (in percent) based on frame size
cairo::rect filter_textoverlay::calculate_sizes( ml::frame_type_ptr& frame,
                                                 const std::string& pos )
{
	double pos_x = 0, pos_y = 0;
	if ( !pos.empty( ) ) {
		std::string x, y;

		std::string::size_type i = pos.find(',');
		if ( i == std::string::npos ) {
			// Missing vertical position
			x = pos;
		} else {
			// Both horizontal and vertical
			x = pos.substr( 0, i );
			y = pos.substr( i + 1 );
		}
		pos_x = calculate_size( frame, false, x );
		pos_y = calculate_size( frame, true, y );
	}

	cairo::rect r;
	r.move( pos_x, pos_y );
	return r;
}

std::string filter_textoverlay::text_to_draw( ml::frame_type_ptr& bg_frame )
{
	ARENFORCE( get_type( ) == "tc" || get_type( ) == "text" );

	if ( get_type( ) == "tc" ) {
		boost::int64_t n;
		pl::pcos::property tc_prop = bg_frame->property( "source_timecode" );
		if (tc_prop.valid()) {
			n = (boost::int64_t) tc_prop.value<int>();
		} else {
			n = (boost::int64_t)bg_frame->get_position( );
		}

		cl::frame_rate::type fr = cl::frame_rate::get_type(
			cl::rational_time(
				bg_frame->get_fps_num( ),
				bg_frame->get_fps_den( ) ) );

		cl::media_time mt = cl::from_frame_number( fr, n );

		cl::time_code tc = mt.to_time_code( fr, false ); // OK, this 'false' and all other completely stupid and insane timecode crap here should be rewritten from scratch.

		// WHOHOO t_string!!! We must now convert yay.
		olib::t_string tstc = tc.to_string( );
		std::string stc = cl::str_util::to_string( tstc );

		return stc;
	} else if ( get_type( ) == "text" ) {
		return get_text( );
	}
	return "[Error]";
}

std::string filter_textoverlay::text_for_extent( ml::frame_type_ptr& bg_frame )
{
	ARENFORCE( get_type( ) == "tc" || get_type( ) == "text" );

	if ( get_type( ) == "tc" ) {
		return "00:00:00:00";
	} else if ( get_type( ) == "text" ) {
		return get_text( );
	}

	return "[Error]";
}

void filter_textoverlay::draw_text( ml::frame_type_ptr& bg_frame )
{
	// Setup all properties used for drawing...
	cairo::rect bounding_box = cairo_surface_->box( );

	// The text to draw
	std::string txt = text_to_draw( bg_frame );
	// The text to use when calculating text extents (usually same as txt)
	std::string txt_extent = text_for_extent( bg_frame );

	double font_size;
	cairo::slant::type font_slant;
	cairo::weight::type font_weight;

	font_size = calculate_size( bg_frame, true, get_font_size( ) );
	font_slant = cairo::slant::from_string( get_font_slant( ) );
	font_weight = cairo::weight::from_string( get_font_weight( ) );

	cairo::origin::type origin;
	cairo::alignment::type alignment;

	origin = cairo::origin::from_string( get_origin( ) );
	alignment = cairo::alignment::from_string( get_alignment( ) );

	// Get position
	cairo::rect pos = calculate_sizes( bg_frame, get_textpos( ) );
	double pos_x = pos.x( ), pos_y = pos.y( );

	// Rebase position based on origin
	bounding_box.rebase( pos_x, pos_y, origin );

	double stroke_width;
	stroke_width = calculate_size( font_size, get_stroke_width( ) );

	// Background box
	cairo::color box_color;
	double box_padding;

	box_color.from_string( get_box_color( ) );
	box_padding = calculate_size( font_size, get_box_padding( ) );

	// Perform drawing...

	cairo_context_->set_alignment( alignment );

	cairo::font font(
		get_font_face( ),
		font_size,
		font_slant,
		font_weight );
	cairo_context_->set_font( font );

	if ( box_color.a( ) ) {
		// Box is visible
		cairo::rect box;

		cairo_context_->move_to( pos_x, pos_y );
		box = cairo_context_->get_string_extents( txt_extent, true );
		box.grow( box_padding );

		cairo_context_->set_color( box_color );
		cairo_context_->fill( box );
	}

	cairo_context_->set_color( get_color( ) );

	cairo_context_->move_to( pos_x, pos_y );
	cairo_context_->text_at( txt, txt_extent );

	bool do_stroke = stroke_width != 0;
	cairo_context_->fill( do_stroke );

	if ( do_stroke ) {
		cairo_context_->move_to( pos_x, pos_y );
		cairo_context_->set_color( get_stroke_color( ) );
		cairo_context_->set_line_width( stroke_width );
		cairo_context_->text_at( txt, txt_extent );
		cairo_context_->stroke( );
	}
}

il::image_type_ptr filter_textoverlay::redraw( ml::frame_type_ptr& bg_frame )
{
	bool needs_clearing = !!cairo_surface_;

	il::image_type_ptr image = bg_frame->get_image( );

	double sar = bg_frame->sar( );

	if ( !cairo_surface_ ) {
		// Create surface
		size_t w = image->width( );
		size_t h = image->height( );

		// We need to create a bigger surface to draw in, so that we
		// stretch it down to frame geometry at composite time.
		// Cairo seems not able to scale down the user coordinates to
		// device coordinates without actually enforcing a smaller
		// clip region. That is really sad.
		if ( sar > 1 )
			w = (double)w * sar + 0.5;
		else if ( sar < 1 )
			h = (double)h / sar + 0.5;

		cairo_surface_ = cairo::surface::make( w, h );
	}

	cairo_context_ = cairo::context::make( cairo_surface_ );

	if ( needs_clearing )
		cairo_context_->clear( );

	draw_text( bg_frame );

	return cairo_surface_->to_image( );
}

void filter_textoverlay::do_fetch( ml::frame_type_ptr& frame )
{
	frame = fetch_from_slot( );

	if ( !frame )
		return; // What do?

	il::image_type_ptr image = frame->get_image( );

	if ( !image )
		return; // What do?

	// TODO: When displaying timecodes, some claimed to be fixed-
	// width-fonts aren't fixed width to cairo, since the last
	// characters aren't always of the same _visible_ size. Hence,
	// when aligning the text to be drawn to the left (or at the
	// center), the text will "jump around". This can be fixed by
	// caching the first bounding box of the text, and force it
	// for all time codes.

	if ( get_dirty( ) || !cairo_context_ || get_type( ) == "tc" ) {
		il::image_type_ptr image = redraw( frame );

		overlay_ = ml::frame_type_ptr( new ml::frame_type( ) );
		overlay_->set_image( image );

		if ( !get_deferred( ) )
			overlay_ = ml::frame_convert( overlay_, L"yuv420p" );

		overlay_->set_sar( 1, 1 );
		overlay_->set_fps( frame->get_fps_num( ), frame->get_fps_den( ) );
		overlay_->set_position( get_position( ) );
		overlay_->set_pts( frame->get_pts( ) );
		overlay_->set_duration( frame->get_duration( ) );
	}
	set_dirty( false );

	// Offer to the internal compositor graph
	frame->set_position( get_position( ) );
	overlay_->set_position( get_position( ) );
	pusher_bg_->push( frame );
	pusher_fg_->push( overlay_ );
	compositor_->seek( get_position( ) );
	frame = compositor_->fetch( );

	// Ensure the position matches
	frame->set_position( get_position( ) );
}


ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_filter_textoverlay( const std::wstring &resource )
{
	return ml::filter_type_ptr( new filter_textoverlay( resource ) );
}


} }

