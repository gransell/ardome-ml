
#include "cairo.hpp"

#include <opencorelib/cl/enforce_defines.hpp>
#include <opencorelib/cl/utilities.hpp>

#include <openimagelib/il/utility.hpp>

namespace cl = olib::opencorelib;
namespace il = olib::openimagelib::il;

#if WIN32
#	define snprintf sprintf_s
#	ifndef M_PI
#		define M_PI 3.14159265358979323846
#	endif
#endif

namespace aml { namespace external { namespace cairo {


color::color()
: r_( 0 ), g_( 0 ), b_( 0 ), a_( 0 )
{
}
color::color( const std::string& clr )
: r_( 0 ), g_( 0 ), b_( 0 ), a_( 0 )
{
	from_string( clr );
}
color::color( double r, double g, double b, double a )
: r_( cl::utilities::clamp< double >( r, 0, 1 ) )
, g_( cl::utilities::clamp< double >( g, 0, 1 ) )
, b_( cl::utilities::clamp< double >( b, 0, 1 ) )
, a_( cl::utilities::clamp< double >( a, 0, 1 ) )
{
}
color::~color()
{
}

void color::from_string( const std::string& clr )
{
	ARENFORCE_MSG(
		( clr.size( ) == 7 || clr.size( ) == 9 ) && clr[0] == '#',
		"Invalid color" );

	double rgba[4] = { 0, 0, 0, 1 };
	size_t n = std::min< size_t >( clr.size( ) - 1, 8 );
	n /= 2;

	for ( size_t i = 0; i < n; ++i ) {
		char buf[3] = {
			clr[ i * 2 + 1 ],
			clr[ i * 2 + 2 ],
			0
		};
		char* p;
		long int num = strtol( buf, &p, 16 );
		ARENFORCE_MSG( !p[0], "Invalid color" );

		rgba[i] = ((double)num) / 255.;
	}

	r_ = rgba[ 0 ];
	g_ = rgba[ 1 ];
	b_ = rgba[ 2 ];
	a_ = rgba[ 3 ];
}

std::string color::to_string( ) const
{
	char buf[10];
	snprintf(
		buf, sizeof buf,
		"#%02x%02x%02x%02x",
		(int)( ( r_ * 255 ) + 0.5 ),
		(int)( ( g_ * 255 ) + 0.5 ),
		(int)( ( b_ * 255 ) + 0.5 ),
		(int)( ( a_ * 255 ) + 0.5 )
		);
	return buf;
}


rect::rect( )
: x1_( 0 ), y1_( 0 ), x2_( 0 ), y2_( 0 )
{
}
rect::rect( double x1, double y1, double x2, double y2 )
: x1_( x1 ), y1_( y1 ), x2_( x2 ), y2_( y2 )
{
}
rect::~rect( )
{
}

double rect::x( ) const
{
	return x1_;
}
double rect::y( ) const
{
	return y1_;
}
double rect::w( ) const
{
	return x2_ - x1_;
}
double rect::h( ) const
{
	return y2_ - y1_;
}
double rect::x2( ) const
{
	return x2_;
}
double rect::y2( ) const
{
	return y2_;
}

rect& rect::move( double x, double y )
{
	set_x( x );
	set_y( y );
	return *this;
}
rect& rect::move_rel( double x, double y )
{
	set_x( x1_ + x );
	set_y( y1_ + y );
	return *this;
}
rect& rect::set_x( double x )
{
	double d = x - x1_;
	x1_ += d;
	x2_ += d;
	return *this;
}
rect& rect::set_y( double y )
{
	double d = y - y1_;
	y1_ += d;
	y2_ += d;
	return *this;
}
rect& rect::resize( double w, double h )
{
	set_w( w );
	set_h( h );
	return *this;
}
rect& rect::set_w( double w )
{
	x2_ = x1_ + w;
	return *this;
}
rect& rect::set_h( double h )
{
	y2_ = y1_ + h;
	return *this;
}
rect& rect::set_x1( double x1 )
{
	x1_ = x1;
	return *this;
}
rect& rect::set_y1( double y1 )
{
	y1_ = y1;
	return *this;
}
rect& rect::set_x2( double x2 )
{
	x2_ = x2;
	return *this;
}
rect& rect::set_y2( double y2 )
{
	y2_ = y2;
	return *this;
}
rect& rect::grow( double sz )
{
	x1_ -= sz;
	y1_ -= sz;
	x2_ += sz;
	y2_ += sz;
	return *this;
}

void rect::rebase( double& x, double& y, origin::type o ) const
{
	switch ( o ) {
		case cairo::origin::top_left:
			break;
		case cairo::origin::top_right:
			x = w( ) - x;
			break;
		case cairo::origin::bottom_right:
			x = w( ) - x;
		case cairo::origin::bottom_left:
			y = h( ) - y;
			break;
	}
}


surface::surface( size_t width, size_t height )
: cs_( NULL )
{
	cs_ = cairo_image_surface_create(
		CAIRO_FORMAT_ARGB32,
		(int)width,
		(int)height);
}
surface::~surface( )
{
	cairo_surface_destroy( cs_ );
}

surface_ptr surface::make( size_t width, size_t height )
{
	return surface_ptr( new surface( width, height ) );
}

il::image_type_ptr surface::to_image( ) const
{
	size_t w = width( );
	size_t h = height( );
	ARENFORCE_MSG( w * h, "Cannot write cairo image with zero size" );

#	if BYTE_ORDER == LITTLE_ENDIAN
#		define PLANE_FORMAT L"b8g8r8a8"
#	else
#		define PLANE_FORMAT L"a8r8g8b8"
#	endif
	il::image_type_ptr image = il::allocate( PLANE_FORMAT, (int)w, (int)h );

	size_t in_stride  = stride( );
	size_t out_stride = (size_t)image->pitch( );

	const unsigned char* in = data( );
	unsigned char*      out = image->data( );

	size_t _stride = std::min< size_t >( in_stride, out_stride );

	if ( _stride == stride( ) && h == (size_t)image->height( ) ) {
		// Copy the whole buffer when the strides are equal
		memcpy( out, in, _stride * h );
	} else {
		for ( size_t y = 0; y < h; ++y ) {
			memcpy( out, in, _stride );
			in += in_stride;
			out += out_stride;
		}
	}

	return image;
}

cairo_surface_t* surface::native( )
{
	return cs_;
}

unsigned char* surface::data( )
{
	return cairo_image_surface_get_data( cs_ );
}
const unsigned char* surface::data( ) const
{
	return cairo_image_surface_get_data( cs_ );
}
size_t surface::width( ) const
{
	return (size_t)cairo_image_surface_get_width( cs_ );
}
size_t surface::height( ) const
{
	return (size_t)cairo_image_surface_get_height( cs_ );
}
size_t surface::stride( ) const
{
	return (size_t)cairo_image_surface_get_stride( cs_ );
}
rect surface::box( ) const
{
	return rect( 0, 0, width( ), height( ) );
}


font::font( const std::string& face, double size, slant::type s, weight::type w )
: face_( face )
, size_( size )
, slant_( (cairo_font_slant_t)s )
, weight_( (cairo_font_weight_t)w )
{
}
font::~font( )
{
}


context::context( surface_ptr surface )
: surface_( surface )
, c_( cairo_create( surface_->native( ) ) )
, alignment_( alignment::right_down )
{
}
context::~context( )
{
	cairo_destroy( c_ );
}

context_ptr context::make( surface_ptr surface )
{
	return context_ptr( new context( surface ) );
}

void context::set_color( double r, double g, double b, double a )
{
	cairo_set_source_rgba( c_, r, g, b, a );
}
void context::set_color( const color& clr )
{
	cairo_set_source_rgba( c_, clr.r( ), clr.g( ), clr.b( ), clr.a( ) );
}
void context::set_color( const std::string& clr )
{
	set_color( color( clr ) );
}
void context::set_line_width( double lw )
{
	cairo_set_line_width( c_, lw );
}

void context::get_cur_pos( double& x, double& y )
{
	cairo_get_current_point( c_, &x, &y );
}

void context::move_to( double x, double y )
{
	cairo_move_to( c_, x, y );
}
void context::line_to( double x, double y )
{
	cairo_line_to( c_, x, y );
}

void context::set_alignment( alignment::type a )
{
	alignment_ = a;
}

void context::set_font( font f )
{
	cairo_select_font_face(
		c_,
		f.face_.c_str( ),
		f.slant_,
		f.weight_);
	cairo_set_font_size(
		c_,
		f.size_);
}

rect context::get_string_extents( const std::string& text, bool absolute )
{
	cairo_text_extents_t extents;
	cairo_text_extents( c_, text.c_str( ), &extents );

	rect r;
	r.move( extents.x_bearing, extents.y_bearing );
	r.resize( extents.width, extents.height );

	// If absolute, respect current position and alignment to figure out
	// where the box is really at
	if ( absolute ) {
		double dx = 0, dy = 0;
		get_alignment_deltas( r, dx, dy );

		// Position the box where we're at
		double x, y;
		get_cur_pos( x, y );

		// Text is based at lower left point, rect's are measured from
		// top left, so move the box up.
		dy -= r.h( );

		r.move( x + dx, y + dy );
	}

	return r;
}

void context::text_at( const std::string& text )
{
	text_at( text, text );
}
void context::text_at( const std::string& text, const std::string& extent_from )
{
	if ( alignment_ == alignment::right_up ) {
		// Simple drawing without caring about text extents
		cairo_text_path( c_, text.c_str( ) );
		return;
	}

	// We need to calculate text extents to base drawing position on this
	rect r = get_string_extents( extent_from );

	double dx = 0, dy = 0;
	get_alignment_deltas( r, dx, dy );

	cairo_rel_move_to( c_, dx, dy );

/**/
//	fill_circle( 5.12 );
/**/

	cairo_text_path( c_, text.c_str( ) );

	cairo_rel_move_to( c_, -dx, -dy );
}
void context::text_at( const std::string& text, double x, double y )
{
	cairo_move_to( c_, x, y );
	text_at( text );
}

void context::circle( double radius )
{
	double x, y;
	get_cur_pos( x, y );

	circle( x, y, radius );
}
void context::circle( double x, double y, double radius )
{
	cairo_arc( c_, x, y, radius, 0, 2*M_PI);
	cairo_close_path( c_ );
}

void context::fill( const rect& r, bool preserve )
{
	cairo_rectangle( c_, r.x( ), r.y( ), r.w( ), r.h( ) );
	fill( preserve );
}
void context::stroke( const rect& r, bool preserve )
{
	cairo_rectangle( c_, r.x( ), r.y( ), r.w( ), r.h( ) );
	stroke( preserve );
}

void context::fill( bool preserve )
{
	if ( preserve )
		cairo_fill_preserve( c_ );
	else
		cairo_fill( c_ );
}
void context::stroke( bool preserve )
{
	if ( preserve )
		cairo_stroke_preserve( c_ );
	else
		cairo_stroke( c_ );
}

void context::clear( )
{
	cairo_save( c_ );

	cairo_set_source_rgba( c_ , 0, 0, 0, 0 );
	cairo_set_operator( c_, CAIRO_OPERATOR_SOURCE );
	cairo_paint( c_ );

	cairo_restore( c_ );
}

surface_ptr context::surface( )
{
	return surface_;
}

void context::get_alignment_deltas( const rect& r, double& dx, double& dy ) const
{
	switch ( alignment_ ) {
		case alignment::right_up:
			break; // Won't happen, catched above
		case alignment::right_middle:
			dy = r.h( ) / 2.;
			break;
		case alignment::right_down:
			dy = r.h( );
			break;
		case alignment::left_up:
			dx = -r.w( );
			break;
		case alignment::left_middle:
			dx = -r.w( );
			dy = r.h( ) / 2.;
			break;
		case alignment::left_down:
			dx = -r.w( );
			dy = r.h( );
			break;
		case alignment::center_up:
			dx = -r.w( ) / 2.;
			break;
		case alignment::center_middle:
			dx = -r.w( ) / 2.;
			dy = r.h( ) / 2.;
			break;
		case alignment::center_down:
			dx = -r.w( ) / 2.;
			dy = r.h( );
			break;
	}
}


} } } // namespace cairo, namespace ext, namespace aml

