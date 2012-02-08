
// il - A image library representation.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

// simple example to exercise the load features of il.

#include <cstdio>
#include <cstdlib>

#ifdef HAVE_CONFIG_H
#include <openlibraries_global_config.hpp>
#endif

#include <GL/glew.h>

#if defined ( WIN32 ) || defined ( HAVE_GL_GLUT_H )
#include <GL/glut.h>
#else
#include <GLUT/glut.h>
#endif

#include <openpluginlib/pl/openpluginlib.hpp>
#include <openpluginlib/pl/utf8_utils.hpp>

#include <openimagelib/il/openimagelib_plugin.hpp>

const int WIN_WIDTH		= 640;
const int WIN_HEIGHT	= 480;

namespace il  = olib::openimagelib::il;
namespace opl = olib::openpluginlib;

namespace
{
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
		
		const opl::wstring& filename_;
	};
}

static GLuint download_photoshop_texture( const boost::filesystem::path& path )
{
	typedef il::image_type::const_pointer			const_pointer;
	typedef il::image_type::size_type				size_type;

	typedef opl::discovery<query_traits> oil_discovery;
	
	oil_discovery plugins( query_traits( opl::to_wstring( path.native_file_string( ).c_str( ) ) ) );
	if( plugins.empty( ) ) return 0; 
	
	oil_discovery::const_iterator i = plugins.begin( );
	boost::shared_ptr<il::openimagelib_plugin> plug = boost::shared_static_cast<il::openimagelib_plugin>( i->create_plugin( "" ) );
	if( !plug ) return 0;
	
	il::image_type_ptr image = plug->load( path );
	if( !image ) return 0;
	
	return 0;
}

static void display( void )
{
#if defined ( WIN32 ) || defined ( HAVE_GL_GL_H )
	glDisable( GL_DEPTH_TEST );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glEnable( GL_DEPTH_TEST );

#ifndef NDEBUG
	GLenum error;
	while( ( error = glGetError( ) ) != GL_NO_ERROR )
		fprintf( stderr, "GLerror:%s\n", gluErrorString( error ) );
#endif
#endif

#if defined ( WIN32 ) || defined ( HAVE_GL_GLUT_H )
	glutSwapBuffers( );
#endif
}

static void idle( void )
{
	glutPostRedisplay( );
}

static void reshape( int w, int h )
{
#if defined ( WIN32 ) || ( defined ( HAVE_GL_GL_H ) && defined ( HAVE_GL_GLU_H ) )
	glViewport( 0, 0, w, h );
	
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity( );
	gluPerspective( 45.0f, ( float ) w / h, 0.01f, 1000.0f );
	
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity( );
	gluLookAt( -1.0f, 0.0f, -10.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f );
#endif
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
#if defined ( WIN32 ) || defined ( HAVE_GL_GLUT_H )
	glutInit( &argc, argv );
	glutInitWindowSize( WIN_WIDTH, WIN_HEIGHT );
	glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGBA );
	glutCreateWindow( "GLTextureDraw" );
	glutDisplayFunc( display );
	glutIdleFunc( idle );
	glutReshapeFunc( reshape );
	glutKeyboardFunc( keyboard );
#endif

#if defined ( WIN32 ) || defined ( HAVE_GL_GLEW_H )
	glewInit( );
#endif

	opl::init( "./plugins" );

	download_photoshop_texture( "./test.psd" );

#if defined ( WIN32 ) || defined ( HAVE_GL_GLUT_H )
	glutMainLoop( );
#endif
}
