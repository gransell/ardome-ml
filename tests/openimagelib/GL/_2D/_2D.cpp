
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
#include <openpluginlib/pl/GL_utility.hpp>

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

static GLuint texid_jpg;
static GLuint texid_bmp;
static GLuint texid_tga;

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

static GLuint download_texture( const boost::filesystem::path& path )
{
	typedef opl::discovery<query_traits> oil_discovery;
	
	oil_discovery plugins( query_traits( opl::to_wstring( path.native_file_string( ).c_str( ) ) ) );
	if( plugins.empty( ) ) return 0; 
	
	oil_discovery::const_iterator i = plugins.begin( );
	boost::shared_ptr<il::openimagelib_plugin> plug = boost::shared_dynamic_cast<il::openimagelib_plugin>( i->create_plugin( "" ) );
	if( !plug ) return 0;

	il::image_type_ptr image = plug->load( path );
	if( !image ) return 0;

	image = il::conform( image, il::cropped | il::flipped | il::flopped );

	image = il::convert( image, L"yuv420p" );
	if( !image ) return 0;

	image = il::convert( image, L"r8g8b8" );
	if( !image ) return 0;

	GLuint id;

	std::pair<int, GLenum> pf = opl::pf_to_gl_format( image->pf( ) );

	glActiveTexture( GL_TEXTURE0 );
	
	glGenTextures( 1, &id );
	glBindTexture( GL_TEXTURE_2D, id );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexImage2D( GL_TEXTURE_2D, 0, pf.first, image->width( ), image->height( ), 0, pf.second, GL_UNSIGNED_BYTE, image->data( ) );

	return id;
}

static void display( void )
{
	glDisable( GL_DEPTH_TEST );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glEnable( GL_DEPTH_TEST );

	glBindTexture( GL_TEXTURE_2D, texid_jpg );
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

	glBindTexture( GL_TEXTURE_2D, texid_bmp );

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

	glBindTexture( GL_TEXTURE_2D, texid_tga );
	
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
	gluLookAt( 0.0f, 0.0f, -6.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f );
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
	glutCreateWindow( "_2D GL Image Loading and Draw" );
	glutDisplayFunc( display );
	glutIdleFunc( idle );
	glutReshapeFunc( reshape );
	glutKeyboardFunc( keyboard );

	glewInit( );
	
	opl::init( plugin_path );
	
	texid_jpg = download_texture( "./test.jpg" );
	texid_bmp = download_texture( "./test.png" );
	texid_tga = download_texture( "./test.bmp" );

	glutMainLoop( );
}
