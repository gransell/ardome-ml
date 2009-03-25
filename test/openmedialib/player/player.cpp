
// ml - A media library representation.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

// simple example to exercise the load features of ml

#include <cstdio>
#include <cstdlib>
#include <string>
#include <iostream>

#ifdef HAVE_CONFIG_H
#include <openlibraries_global_config.hpp>
#endif

#include <openpluginlib/pl/openpluginlib.hpp>
#include <openpluginlib/pl/utf8_utils.hpp>

#include <openimagelib/il/openimagelib_plugin.hpp>
#include <openmedialib/ml/openmedialib_plugin.hpp>

#include <GL/glew.h>

#if defined ( WIN32 ) || defined ( HAVE_GL_GLUT_H )
#include <GL/glut.h>
#else
#include <GLUT/glut.h>
#endif

namespace ml = olib::openmedialib::ml;
namespace il = olib::openimagelib::il;
namespace pl = olib::openpluginlib;

ml::input_type_ptr orig;
GLuint texture_id;

#ifdef WIN32
	const pl::string oml_plugin_path( "./plugins" );
	const pl::string oil_plugin_path( "./plugins" );
#else
	const pl::string oil_plugin_path( OPENIMAGELIB_PLUGINS );
	const pl::string oml_plugin_path( OPENMEDIALIB_PLUGINS );
#endif

int WIN_WIDTH = 640;
int WIN_HEIGHT = 480;
int speed = 100;
int duration = 0;


static GLenum pixelformat_to_gl( pl::wstring pf )
{
	if( pf == L"r8g8b8" )
		return GL_RGB;
	else if( pf == L"b8g8r8" )
		return GL_BGR;
	else if( pf == L"r8g8b8a8" )
		return GL_RGBA;
	else
		return 0;
}

void calculate_dimensions( ml::frame_type_ptr frame, int &phy_w, int &phy_h, int &req_w, int &req_h )
{
	ml::image_type_ptr image = frame->get_image( );

	// This is the absolute WxH we have
	int abs_w = WIN_WIDTH;
	int abs_h = WIN_HEIGHT;

	// This is the physcial WxH of the image
	double ar = frame->aspect_ratio( );

	phy_w = image->width( );
	phy_h = image->height( );

	// Calculate the required display WxH
	req_h = abs_h;
	req_w = int( req_h * ar );

	// Scale up or down to fit
	if ( req_w > abs_w )
	{
		req_w = abs_w;
		req_h = int( req_w / ar );
	}
}

bool texture_target( size_t width, size_t height, GLenum& target, float& tex_w, float& tex_h )
{
	if( GLEW_VERSION_2_0 || GLEW_ARB_texture_non_power_of_two || ( !( width & ( width - 1 ) ) ) && ( !( height & ( height - 1 ) ) ) )
	{
		// ???? Goncalo - could you check this? Lifted and slightly changed from the jahshaka mlt edit module
		// what's below is correct (in theory that is). I've seen cases where it fails, but this shouldn't be one of them - Goncalo.
		target = GL_TEXTURE_2D;
		tex_w = 1.0f, tex_h = 1.0f;
		return true;
	}
	else if( GLEW_ARB_texture_rectangle )
	{
		target = GL_TEXTURE_RECTANGLE_ARB;
		tex_w = width, tex_h = height;
		return true;
	}
	else if( GLEW_EXT_texture_rectangle )
	{
		target = GL_TEXTURE_RECTANGLE_EXT;
		tex_w = width, tex_h = height;
		return true;
	}

	return false;
}

static void display( void )
{

	
	glDisable( GL_DEPTH_TEST );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glEnable( GL_DEPTH_TEST );

	ml::frame_type_ptr frame = orig->fetch( );




	if (frame!=0)
	{
	ml::image_type_ptr image = frame->get_image( );

	if (image!=0)
	{

	duration =  int( frame->get_duration( ) * 1000 );
	
	int phy_w, phy_h, req_w, req_h;
	calculate_dimensions( frame, phy_w, phy_h, req_w, req_h );

	il::image_type_ptr new_im = il::convert( image, L"r8g8b8" );
	new_im = il::conform( new_im, il::cropped | il::flipped );



	
	float tex_w, tex_h;
	GLenum target;
	if ( texture_target( phy_w, phy_h, target, tex_w, tex_h ) )
	{
   		glClear( GL_COLOR_BUFFER_BIT );

		glDisable( GL_TEXTURE_2D );
		glDisable( GL_DEPTH_TEST );
		glDepthMask( 0 );

		glEnable( target );

		glBindTexture( target, texture_id );
		glTexParameteri( target, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( target, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

		glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );
		glTexImage2D( target, 0, 3, phy_w, phy_h, 0, pixelformat_to_gl( new_im->pf( ) ), GL_UNSIGNED_BYTE, new_im->data( ) );

		glMatrixMode( GL_PROJECTION );
		glPushMatrix( );
		glLoadIdentity( );
		gluOrtho2D( 0.0f, WIN_WIDTH, 0.0f, WIN_HEIGHT );
				
		glMatrixMode( GL_MODELVIEW );
		glPushMatrix( );
		glLoadIdentity( );
		
		float off_x = 0.5 * ( WIN_WIDTH - req_w );
		float off_y = 0.5 * ( WIN_HEIGHT - req_h );
		glTranslatef( off_x, off_y, 0.0f );

		float zoom_w = ( float ) req_w / ( float ) phy_w; 
		float zoom_h = ( float ) req_h / ( float ) phy_h;
		glScalef( zoom_w, zoom_h, 1.0f );

		glBegin( GL_QUADS );
			glTexCoord2f( 0.0f, 0.0f );
			glVertex2f( 0.0f, 0.0f );
			glTexCoord2f( tex_w, 0.0f );
			glVertex2f( phy_w, 0.0f );
			glTexCoord2f( tex_w, tex_h );
			glVertex2f( phy_w, phy_h );
			glTexCoord2f( 0.0f, tex_h );
			glVertex2f( 0.0f, phy_h );
		glEnd( );

		glMatrixMode( GL_MODELVIEW );
		glPopMatrix( );

		glMatrixMode( GL_PROJECTION );
		glPopMatrix( );

		glDisable( target );

		glDepthMask( 1 );
		glEnable( GL_DEPTH_TEST );
		glEnable( GL_TEXTURE_2D );
	}
	else
	{
		glDisable( GL_TEXTURE_2D );
		glDisable( GL_DEPTH_TEST );
		glDepthMask( 0 );

		// Display the image at the correct aspect ratio and scaled to fill the core as much as possible
   		glClear( GL_COLOR_BUFFER_BIT );
   		glMatrixMode( GL_MODELVIEW );
		glPixelZoom( ( ( float )req_w / phy_w ), ( ( float )req_h / phy_h ) );
   		glDrawPixels( phy_w, phy_h, pixelformat_to_gl( new_im->pf( ) ), GL_UNSIGNED_BYTE, new_im->data( ) );

		// Reverses the actions above
		glDepthMask( 1 );
		glEnable( GL_DEPTH_TEST );
		glEnable( GL_TEXTURE_2D );
	}

#ifndef NDEBUG
	GLenum error;
	while( ( error = glGetError( ) ) != GL_NO_ERROR )
		fprintf( stderr, "GLerror:%s\n", gluErrorString( error ) );
#endif

	glutSwapBuffers( );

	

	//orig->seek( speed, true );
	//speed+=10000;
	//printf("seek en play\n");
	}
	}
	glutPostRedisplay( );
}

static void timer( int )
{
	glutPostRedisplay( );
//	glutTimerFunc( duration, timer, 0 );
}

static void reshape( int w, int h )
{
	WIN_WIDTH = w;
	WIN_HEIGHT = h;

	glViewport( 0, 0, w, h );
	
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity( );
	gluPerspective( 45.0f, ( float ) w / h, 0.01f, 1000.0f );
	
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity( );
	gluLookAt( 0.0f, 0.0f, -3.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f );
}

static void keyboard( unsigned char key, int /*x*/, int /*y*/ )
{
	
	switch( key )
	{
		case 27:
			
			orig->~input_type();
			exit( 0 );
			break;

		case ' ':
			int d;
			d=orig->get_frames();
			printf( "frames = %d\n", d);
			break;

		case 'q':
			//speed = 0;
			orig->seek( orig->get_position() - (100000000), true );//10 seconds
			break;

		case 'w':
			//speed = 0;
			orig->seek( orig->get_position() + (100000000), true );//10 seconds
			break;

		default:
			break;
	}

}

void play( ml::input_type_ptr input )
{
	
	//at the moment, the override function "get_frames()" returns 0....
	//if ( input->get_frames( ) == 0 ) 
	//{ 
	//	std::cerr << "Nothing to play" << std::endl; return; 
	//}

	orig = input;

	ml::frame_type_ptr frame = input->fetch( );
	ml::image_type_ptr image = frame->get_image( );

	printf("pf: %s\n", opl::to_string(image->pf()).c_str());
	printf("size: %ld\n", image->size());

	printf("depth: %d\n", image->depth());
	printf("count: %d\n", image->count());
	printf("block_size: %d\n", image->block_size());
	printf("bitdepth: %d\n", image->bitdepth());

	printf("is_cubemap: %s\n", image->is_flipped() ? "TRUE" : "FALSE");
	printf("is_volume: %s\n", image->is_flopped() ? "TRUE" : "FALSE");
	printf("is_float: %s\n", image->is_writable() ? "TRUE" : "FALSE");
	printf("is_flipped: %s\n", image->is_flipped() ? "TRUE" : "FALSE");
	printf("is_flopped: %s\n", image->is_flopped() ? "TRUE" : "FALSE");
	printf("is_writable: %s\n", image->is_writable() ? "TRUE" : "FALSE");
	printf("pts: %f\n", image->pts());
	printf("position: %d\n", image->position());
	printf("field order: %d\n", image->field_order());



	printf("is_cropped: %s\n", image->is_cropped());
	printf("crop x: %d, y: %d, w: %d, h: %d\n", image->get_crop_x(), image->get_crop_y(), image->get_crop_w(), image->get_crop_h());
	printf("plane count: %d\n", image->plane_count());
	for(int count = 0;
		count < image->plane_count();
		count++)
	{
		printf("\nplane %d:\n", count);
		printf("width: %d\n", image->width(count));
		printf("height: %d\n", image->height(count));
		printf("pitch: %d\n", image->pitch(count));
		printf("offset: %d\n", image->offset(count));
		printf("linesize: %d\n", image->linesize(count));
		printf("data: 0x%08x\n", image->data(count));
	}

	printf("\n");

	int phy_w, phy_h, req_w, req_h;
	calculate_dimensions( frame, phy_w, phy_h, req_w, req_h );

	float tex_w, tex_h;
	GLenum target;
	if ( texture_target( phy_w, phy_h, target, tex_w, tex_h ) )
	{
		glActiveTexture( GL_TEXTURE0 );
	
		glGenTextures( 1, &texture_id );
		glBindTexture( target, texture_id );
		glTexParameteri( target, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexParameteri( target, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		glTexParameteri( target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	}


	glutMainLoop( );
}

int main( int argc, char* argv[ ] )
{
	// Initialise the glut environment
	glutInit( &argc, argv );
	glutInitWindowSize( WIN_WIDTH, WIN_HEIGHT );
	glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGB );
	glutCreateWindow( "GLTextureDraw" );
	glutDisplayFunc( display );

	glutTimerFunc( 100, timer, 0 );
	glutReshapeFunc( reshape );
	glutKeyboardFunc( keyboard );

	// Initialise the glew environment
	glewInit( );



	// Load both oil and oml plugins since oml uses oil
	pl::init( (argc >= 2) ? argv[ 2 ] : "" );

	if ( argc > 1 )
	{
		ml::input_type_ptr input = ml::create_input( argv[ 1 ] );
		if ( input != 0 )
		{
			play( input );
		}
		else
			std::cerr << "No plugins found for " << argv[ 1 ] << std::endl;
	}

	return 0;
}

