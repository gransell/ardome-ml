
// il - A image library representation.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

// simple example to exercise the load features of il (rgb24 only).

#include <cstdio>
#include <cstdlib>
#include <string>

#ifdef HAVE_CONFIG_H
#include <openlibraries_global_config.hpp>
#endif

#include <openpluginlib/pl/openpluginlib.hpp>
#include <openpluginlib/pl/utf8_utils.hpp>

#include <openimagelib/il/openimagelib_plugin.hpp>

#include <GL/glew.h>

#if defined ( WIN32 ) || defined ( HAVE_GL_GLUT_H )
#include <GL/glut.h>
#else
#include <GLUT/glut.h>
#endif

const int WIN_WIDTH		= 640;
const int WIN_HEIGHT	= 480;
const int IMG_SIZE		= 144;

namespace il  = olib::openimagelib::il;
namespace opl = olib::openpluginlib;

namespace
{
	GLuint texid_jpg;

	int crop_x = 0;
	int crop_y = 0;
	int orig_w, orig_h;
	int cycle = 0;
	int offset_x = 1;
	int offset_y = 0;

	ml::image_type_ptr orig;
	
#ifdef WIN32
	const opl::string plugin_path( "./plugins" );
#else
	const opl::string plugin_path( OPENIMAGELIB_PLUGINS );
#endif

	struct query_traits
	{
		query_traits( const opl::wstring& filename )
			: filename_( filename )
		{ }
		
		opl::wstring libname( ) const
		{ return L"openimagelib"; }
		
		opl::wstring to_match( ) const
		{ return filename_; }

		opl::wstring type( ) const
		{ return L""; }
		
		int merit( ) const
		{ return 0; }
		
		opl::wstring filename_;
	};
}

static GLenum pixelformat_to_gl( opl::wstring pf )
{
	if( pf == L"r8g8b8" )
	{
		return GL_RGB;
	}
	else if( pf == L"b8g8r8" )
	{
		return GL_BGR;
	}
	else if( pf == L"r8g8b8a8" )
	{
		return GL_RGBA;
	}
	else
	{
		return 0;
	}
}

static GLuint download_texture( const boost::filesystem::path& path )
{
	typedef opl::discovery<query_traits> oil_discovery;
	
	oil_discovery plugins( query_traits( opl::to_wstring( path.native_file_string( ).c_str( ) ) ) );
	if( plugins.empty( ) ) return 0; 
	
	oil_discovery::const_iterator i = plugins.begin( );
	boost::shared_ptr<il::openimagelib_plugin> plug = boost::shared_dynamic_cast<il::openimagelib_plugin>( i->create_plugin( "" ) );
	if( !plug ) return 0;

	ml::image_type_ptr image = plug->load( path );
	if( !image ) return 0;

	orig   = image;
	orig_w = image->width( );
	orig_h = image->height( );

	GLuint id;

	glActiveTexture( GL_TEXTURE0 );
	
	glGenTextures( 1, &id );
	glBindTexture( GL_TEXTURE_2D, id );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB8, image->width( ), image->height( ), 0, pixelformat_to_gl( image->pf( ) ), GL_UNSIGNED_BYTE, image->data( ) );

	return id;
}

static void display( void )
{
	glDisable( GL_DEPTH_TEST );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glEnable( GL_DEPTH_TEST );

	orig->crop_clear( );
	orig->crop( crop_x, crop_y, IMG_SIZE, IMG_SIZE );
	ml::image_type_ptr new_im = il::conform( orig, il::cropped | il::flipped | il::flopped );

	glBindTexture( GL_TEXTURE_2D, texid_jpg );
	glEnable( GL_TEXTURE_2D );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB8, new_im->width( ), new_im->height( ), 0, pixelformat_to_gl( new_im->pf( ) ), GL_UNSIGNED_BYTE, new_im->data( ) );

	glPushMatrix( );

	glBegin( GL_QUADS );
		glMultiTexCoord2f( GL_TEXTURE0, 0.0f, 0.0f );
		glVertex3f( -1.0f, -1.0f, -1.0f );
		glMultiTexCoord2f( GL_TEXTURE0, 1.0f, 0.0f );
		glVertex3f(  1.0f, -1.0f, -1.0f );
		glMultiTexCoord2f( GL_TEXTURE0, 1.0f, 1.0f );
		glVertex3f(  1.0f,  1.0f, -1.0f );
		glMultiTexCoord2f( GL_TEXTURE0, 0.0f, 1.0f );
		glVertex3f( -1.0f,  1.0f, -1.0f );
	glEnd( );
	
	glPopMatrix( );

#ifndef NDEBUG
	GLenum error;
	while( ( error = glGetError( ) ) != GL_NO_ERROR )
		fprintf( stderr, "GLerror:%s\n", gluErrorString( error ) );
#endif

	glutSwapBuffers( );
}

static void timer( int )
{
	crop_x += offset_x;
	crop_y += offset_y;
	
	if( crop_x + IMG_SIZE > orig_w || crop_x < 0 || crop_y + IMG_SIZE > orig_h || crop_y < 0 )
		cycle ++;
	
	switch ( cycle % 10 )
	{
		case 0:		offset_x = 1; offset_y = 0; break;
		case 1:		offset_x = -1; offset_y = 0; break;
		case 2:		offset_x = 1; offset_y = 1; break;
		case 3:		offset_x = -1; offset_y = 0; break;
		case 4:		offset_x = 1; offset_y = -1; break;
		case 5:		offset_x = 0; offset_y = 1; break;
		case 6:		offset_x = -1; offset_y = -1; break;
		case 7:		offset_x = 1; offset_y = 0; break;
		case 8:		offset_x = 0; offset_y = 1; break;
		case 9:		offset_x = -1; offset_y = -1; break;
	}

	if ( crop_x < 0 ) crop_x = 0;
	if ( crop_y < 0 ) crop_y = 0;
	if ( crop_x + IMG_SIZE > orig_w ) crop_x = orig_w - IMG_SIZE;
	if ( crop_y + IMG_SIZE > orig_h ) crop_y = orig_h - IMG_SIZE;

	glutPostRedisplay( );
	glutTimerFunc( 10, timer, 0 );
}

static void reshape( int w, int h )
{
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
			orig = ml::image_type_ptr( );
			exit( 0 );
			break;
		
		default:
			break;
	}
}

int main( int argc, char* argv[ ] )
{
	glutInit( &argc, argv );
	glutInitWindowSize( WIN_WIDTH, WIN_HEIGHT );
	glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGBA );
	glutCreateWindow( "GLTextureDraw" );
	glutDisplayFunc( display );
	glutTimerFunc( 10, timer, 0 );
	glutReshapeFunc( reshape );
	glutKeyboardFunc( keyboard );

	glewInit( );
	
	opl::init( plugin_path );
	
	texid_jpg = download_texture( boost::filesystem::path(argv[ 1 ] != NULL ? argv[ 1 ] : "./test.jpg", boost::filesystem::native) );

	glutMainLoop( );
}
