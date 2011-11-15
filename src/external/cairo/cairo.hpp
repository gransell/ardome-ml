
// Cairo wrapper classes
//
// Copyright (C) 2011 Ardendo
// Released under the terms of the LGPL.
//

#include "cairo_types.hpp"
#include <string>

#include <openimagelib/il/basic_image.hpp>

namespace aml { namespace external { namespace cairo {


/*
 * This file contains helper classes to the cairo library
 * Note;
 *     All text functions use UTF-8 only. No horrendous wide char strings or
 *     any such thing. Convert your strings if they aren't UTF-8!
 */

class CAIRO_API color {
public:
	color();
	color( const std::string& clr );
	color( double r, double g, double b, double a = 1 );
	virtual ~color();

	void from_string( const std::string& clr );

	double r( ) const { return r_; }
	double g( ) const { return g_; }
	double b( ) const { return b_; }
	double a( ) const { return a_; }

	std::string to_string( ) const;

private:
	double r_, g_, b_, a_;
};

class CAIRO_API rect {
public:
	rect( );
	rect( double x1, double y1, double x2, double y2 );
	virtual ~rect( );

	double x( ) const;
	double y( ) const;
	double w( ) const;
	double h( ) const;
	double x2( ) const;
	double y2( ) const;

	rect& move( double x, double y );     // Moves the whole rectangle
	rect& move_rel( double x, double y ); // Moves the whole rectangle
	rect& set_x( double x );              // Moves the whole rectangle
	rect& set_y( double y );              // Moves the whole rectangle
	rect& resize( double w, double h );   // Resizes the rectangle
	rect& set_w( double w );              // Resizes the rectangle
	rect& set_h( double h );              // Resizes the rectangle
	rect& set_x1( double x1 );            // Only changes x1
	rect& set_y1( double y1 );            // Only changes y1
	rect& set_x2( double x2 );            // Only changes x2
	rect& set_y2( double y2 );            // Only changes y2
	rect& grow( double sz );              // Grow in all directions

	void rebase( double& x, double& y, origin::type o ) const;

private:
	double x1_;
	double y1_;
	double x2_;
	double y2_;
};

class CAIRO_API surface {
public:
	surface( size_t width, size_t height );
	surface(  );
	virtual ~surface( );

	static surface_ptr make( size_t width, size_t height );

	olib::openimagelib::il::image_type_ptr to_image( ) const;

	cairo_surface_t* native( );

	unsigned char* data( );
	const unsigned char* data( ) const;
	size_t width( ) const;
	size_t height( ) const;
	size_t stride( ) const;

	rect box( ) const;

private:
	cairo_surface_t* cs_;
};

class CAIRO_API font {
public:
	font( const std::string& face, double size, slant::type s = slant::normal, weight::type w = weight::normal );
	virtual ~font( );

private:
	friend class context;
	const std::string face_;
	double size_;
	cairo_font_slant_t slant_;
	cairo_font_weight_t weight_;
};

// A context is a drawing state on which we can draw text, shapes and images.
class CAIRO_API context {
public:
	context( surface_ptr surface );
	virtual ~context( );

	static context_ptr make( surface_ptr surface );

	void set_color( double r, double g, double b, double a = 1 );
	void set_color( const color& clr );
	void set_color( const std::string& clr );
	void set_line_width( double lw );

	void get_cur_pos( double& x, double& y );
	void move_to( double x, double y );
	void line_to( double x, double y );

	void set_alignment( alignment::type a );
	void set_font( font f );
	rect get_string_extents( const std::string& text, bool absolute = false );
	void text_at( const std::string& text );
	void text_at( const std::string& text, const std::string& extent_from );
	void text_at( const std::string& text, double x, double y );

	void circle( double radius );
	void circle( double x, double y, double radius );

	void fill( const rect& r, bool preserve = false );
	void stroke( const rect& r, bool preserve = false );

	void fill( bool preserve = false );
	void stroke( bool preserve = false );

	void clear( );


	surface_ptr surface( );

private:
	void get_alignment_deltas( const rect& r, double& dx, double& dy ) const;

	surface_ptr surface_;
	cairo_t* c_;
	alignment::type alignment_;
};


} } } // namespace cairo, namespace external, namespace aml

