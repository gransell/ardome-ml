
#include "cairo.hpp"

namespace aml { namespace external { namespace cairo {


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

cairo_surface_t* surface::native( )
{
	return cs_;
}

unsigned char* surface::data( )
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

double rect::x( )
{
	return x1_;
}
double rect::y( )
{
	return x1_;
}
double rect::w( )
{
	return x2_ - x1_;
}
double rect::h( )
{
	return y2_ - y1_;
}
double rect::x2( )
{
	return x2_;
}
double rect::y2( )
{
	return y2_;
}

rect& rect::move( double x, double y )
{
	set_x( x );
	set_y( y );
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
, origin_( origin::top_left )
, alignment_( alignment::right_down )
{
}
context::~context( )
{
	cairo_destroy( c_ );
}

void context::set_color( double r, double g, double b, double a )
{
	cairo_set_source_rgba( c_, r, g, b, a );
}
void context::set_line_width( double lw )
{
	cairo_set_line_width( c_, lw );
}

void context::move_to( double x, double y )
{
	cairo_move_to( c_, x, y );
}
void context::line_to( double x, double y )
{
	cairo_line_to( c_, x, y );
}

void context::set_origin( origin::type o )
{
	origin_ = o;
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

rect context::get_string_extents( const std::string& text )
{
	cairo_text_extents_t extents;
	cairo_text_extents( c_, text.c_str( ), &extents );

	rect r;
	r.move( extents.x_bearing, extents.y_bearing );
	r.resize( extents.width, extents.height );

	return r;
}

void context::text_at( const std::string& text )
{
	if ( origin_ != origin::top_left || alignment_ != alignment::right_down ) {
	}

	cairo_text_path( c_, text.c_str( ) );
}
void context::text_at( const std::string& text, double x, double y )
{
	cairo_move_to( c_, x, y );
	text_at( text );
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


} } } // namespace cairo, namespace ext, namespace aml

