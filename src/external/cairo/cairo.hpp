
// Cairo wrapper classes
//
// Copyright (C) 2011 Ardendo
// Released under the terms of the LGPL.
//

#include "cairo_types.hpp"
#include <string>

namespace aml { namespace external { namespace cairo {


/*
 * This file contains helper classes to the cairo library
 * Note;
 *     All text functions use UTF-8 only. No horrendous wide char strings or
 *     any such thing. Convert your strings if they aren't UTF-8!
 */

class surface {
public:
	surface( size_t width, size_t height );
	surface(  );
	virtual ~surface( );

	cairo_surface_t* native( );

	unsigned char* data( );
	size_t width( ) const;
	size_t height( ) const;
	size_t stride( ) const;

private:
	cairo_surface_t* cs_;
};

class rect {
public:
	rect( );
	rect( double x1, double y1, double x2, double y2 );
	virtual ~rect( );

	double x( );
	double y( );
	double w( );
	double h( );
	double x2( );
	double y2( );

	rect& move( double x, double y );   // Moves the whole rectangle
	rect& set_x( double x );            // Moves the whole rectangle
	rect& set_y( double y );            // Moves the whole rectangle
	rect& resize( double w, double h ); // Resizes the rectangle
	rect& set_w( double w );            // Resizes the rectangle
	rect& set_h( double h );            // Resizes the rectangle
	rect& set_x1( double x1 );          // Only changes x1
	rect& set_y1( double y1 );          // Only changes y1
	rect& set_x2( double x2 );          // Only changes x2
	rect& set_y2( double y2 );          // Only changes y2

private:
	double x1_;
	double y1_;
	double x2_;
	double y2_;
};

class font {
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

class context {
public:
	context( surface_ptr surface );
	virtual ~context( );

	void set_color( double r, double g, double b, double a = 1 );
	void set_line_width( double lw );

	void move_to( double x, double y );
	void line_to( double x, double y );

	void set_origin( origin::type o );
	void set_alignment( alignment::type a );
	void set_font( font f );
	rect get_string_extents( const std::string& text );
	void text_at( const std::string& text );
	void text_at( const std::string& text, double x, double y );

	void fill( bool preserve = false );
	void stroke( bool preserve = false );

private:
	surface_ptr surface_;
	cairo_t* c_;
	origin::type origin_;
	alignment::type alignment_;
};


} } } // namespace cairo, namespace external, namespace aml

