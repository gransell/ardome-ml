
// il - A image library representation.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

// simple example to exercise the load features of il.

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

const int WIN_WIDTH	 = 640;
const int WIN_HEIGHT = 480;

namespace il  = olib::openimagelib::il;
namespace opl = olib::openpluginlib;

static GLuint texid_rgb;
static GLuint texid_bw;
static GLuint texid_la;
static GLuint texid_rgba;

namespace
{
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
	else if( pf == L"r8g8b8a8" )
	{
		return GL_RGBA;
	}
	else if( pf == L"l8a8" )
	{
		return GL_LUMINANCE_ALPHA;
	}
	else if( pf == L"l8" )
	{
		return GL_LUMINANCE;
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

	image = il::conform( image, il::cropped | il::flipped | il::flopped );

	GLuint id;

	glActiveTexture( GL_TEXTURE0 );
	
	glGenTextures( 1, &id );
	glBindTexture( GL_TEXTURE_2D, id );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, image->width( ), image->height( ), 0, pixelformat_to_gl( image->pf( ) ), GL_UNSIGNED_BYTE, image->data( ) );

	return id;
}

static void display( void )
{
	glDisable( GL_DEPTH_TEST );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glEnable( GL_DEPTH_TEST );

	glBindTexture( GL_TEXTURE_2D, texid_rgb );
	glEnable( GL_TEXTURE_2D );

	glPushMatrix( );
	glTranslatef( 2.0f, 0.0f, 0.0f );

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

	glBindTexture( GL_TEXTURE_2D, texid_bw );

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

	glBindTexture( GL_TEXTURE_2D, texid_la );
	
	glPushMatrix( );
	glTranslatef( -2.0f, 0.0f, 0.0f );

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

	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	glBindTexture( GL_TEXTURE_2D, texid_rgba );
	
	glPushMatrix( );
	glTranslatef( 0.0f, 2.0f, 0.0f );

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
	
	glDisable( GL_BLEND );

#ifndef NDEBUG
	GLenum error;
	while( ( error = glGetError( ) ) != GL_NO_ERROR )
		fprintf( stderr, "GLerror:%s\n", gluErrorString( error ) );
#endif

	glutSwapBuffers( );
}

static void idle( void )
{
	glutPostRedisplay( );
}

static void reshape( int w, int h )
{
	glViewport( 0, 0, w, h );
	
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity( );
	gluPerspective( 45.0f, ( float ) w / h, 0.01f, 1000.0f );
	
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity( );
	gluLookAt( 0.0f, 0.0f, -8.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f );
}

static void keyboard( unsigned char key, int /*x*/, int /*y*/ )
{
	switch( key )
	{
		case 27:
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
	glutCreateWindow( "GL Texture SGI Format Unit Test" );
	glutDisplayFunc( display );
	glutIdleFunc( idle );
	glutReshapeFunc( reshape );
	glutKeyboardFunc( keyboard );

	glewInit( );
	
	opl::init( plugin_path );
	
	texid_rgb  = download_texture( "./test.rgb" );
	texid_bw   = download_texture( "./test.bw" );
	texid_la   = download_texture( "./test.la" );
	texid_rgba = download_texture( "./test.rgba" );

	glutMainLoop( );
}
