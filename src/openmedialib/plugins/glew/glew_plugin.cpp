
// glew - A glew plugin to ml.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifdef HAVE_CONFIG_H
#include <openlibraries_global_config.hpp>
#endif

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openpluginlib/pl/string.hpp>

#ifdef WIN32
#include <windows.h>
#endif // WIN32

#include <iostream>
#include <cstdlib>
#include <vector>
#include <deque>
#include <string>

#include <boost/thread/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/recursive_mutex.hpp>

#include <GL/glew.h>

#if defined ( WIN32 ) || defined ( HAVE_GL_GLUT_H )
#include <GL/glut.h>
#else
#include <GLUT/glut.h>
#endif

namespace opl = olib::openpluginlib;
namespace plugin = olib::openmedialib::ml;


namespace olib { namespace openmedialib { namespace ml { 

class ML_PLUGIN_DECLSPEC glew_store : public store_type, public store_keyboard_feedback
{

	private:
		glew_store( frame_type_ptr frame ) 
			: store_type( )
			, store_keyboard_feedback( )
			, WIN_WIDTH( 640 )
			, WIN_HEIGHT( 480 )
			, duration( 10 )
			, keyboard_handler_( 0 )
			, done_( false )
		{
			duration = ( frame->get_fps_den( ) * 1000 ) / frame->get_fps_num( );
			frames_.push_back( frame );
		}

	public:
		virtual ~glew_store( )
		{
			audio_ = store_type_ptr( );
		}

		static glew_store *get_instance( )
		{
			return instance_;
		}

		static glew_store *get_instance( frame_type_ptr frame )
		{
			if ( instance_ == 0 )
			{
				instance_ = new glew_store( frame );
				new boost::thread( &thread );
			}
			return instance_;
		}

		static void timer( int val )
		{
			get_instance( )->inner_timer( val );
		}

		inline void inner_timer( int )
		{
			if ( !done_ )
			{
				glutTimerFunc( duration, timer, 0 );
				glutPostRedisplay( );

				boost::mutex::scoped_lock scoped_lock( mutex_ );

				if ( frames_.size( ) > 0 )
					frames_.pop_front( );

				if ( frames_.size( ) > 0 )
				{
					frame_type_ptr frame = *frames_.begin( );

					// Push the audio
					if ( audio_ != 0 )
						audio_->push( frame );

					// Make sure the audio is only played once
					frame->set_audio( audio_type_ptr( ) );
				}

				notify( );
			}
			else
			{
				boost::mutex::scoped_lock scoped_lock( mutex_ );
				notify( );
			}
		}

		static void reshape( int w, int h )
		{
			get_instance( )->inner_reshape( w, h );
		}

		inline void inner_reshape( int w, int h )
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

		GLenum pixelformat_to_gl( olib::t_string pf )
		{
			if( pf == _CT("r8g8b8") )
				return GL_RGB;
			else if( pf == _CT("b8g8r8") )
				return GL_BGR;
			else if( pf == _CT("r8g8b8a8") )
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
			if( ( GLEW_VERSION_2_0 || GLEW_ARB_texture_non_power_of_two || !( width & ( width - 1 ) ) ) && ( !( height & ( height - 1 ) ) ) )
			{
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
			else if( GLEW_NV_texture_rectangle )
			{
				target = GL_TEXTURE_RECTANGLE_NV;
				tex_w = width, tex_h = height;
				return true;
			}
		
			return false;
		}

		static void display( void )
		{
			get_instance( )->inner_display( );
		}

		frame_type_ptr get_frame( )
		{
			boost::mutex::scoped_lock scoped_lock( mutex_ );
			if ( frames_.size( ) == 0 ) return frame_type_ptr( );
			frame_type_ptr frame = frames_[ 0 ];
			return frame;
		}

		void wait( )
		{
			boost::mutex::scoped_lock lock (cond_mutex_);
			cond_.wait( lock );
		}

		void notify( )
		{
			cond_.notify_all( );
		}

		inline void inner_display( void )
		{
			glDisable( GL_DEPTH_TEST );
			glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
			glEnable( GL_DEPTH_TEST );

			frame_type_ptr frame = get_frame( );
			while( !done_ && frame == 0 )
			{
				wait( );
				frame = get_frame( );
			}

			if ( done_ ) exit( 0 );

			image_type_ptr image = frame->get_image( );

			if ( image != 0 )
			{
				duration = ( frame->get_fps_den( ) * 1000 ) / frame->get_fps_num( );
	
				int phy_w, phy_h, req_w, req_h;
				calculate_dimensions( frame, phy_w, phy_h, req_w, req_h );

				// Convert to a format which the GPU knows
				image_type_ptr new_im = ml::image::convert( image, _CT("b8g8r8") );

				float tex_w, tex_h;
				GLenum target;
				if ( texture_target( phy_w, phy_h, target, tex_w, tex_h ) )
				{
					// Conform to cropped and not flipped or flopped (optimal for video playout)
					new_im = ml::image::conform( new_im, ml::image::cropped );
					frame->set_image( new_im );

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
						glVertex2f( 0.0f, phy_h );
						glTexCoord2f( tex_w, 0.0f );
						glVertex2f( phy_w, phy_h );
						glTexCoord2f( tex_w, tex_h );
						glVertex2f( phy_w, 0.0f );
						glTexCoord2f( 0.0f, tex_h );
						glVertex2f( 0.0f, 0.0f );
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
					// Since we don't have a texture, crop, flip and don't flop
					new_im = ml::image::conform( new_im, ml::image::cropped | ml::image::flipped );
					frame->set_image( new_im );

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
			}

#	ifndef NDEBUG
			GLenum error;
			while( ( error = glGetError( ) ) != GL_NO_ERROR )
				fprintf( stderr, "GLerror:%s\n", gluErrorString( error ) );
#	endif

			glutSwapBuffers( );
		}

		virtual void register_keyboard_handler( store_keyboard_handler *handler )
		{
			keyboard_handler_ = handler;
		}

		static void keyboard( unsigned char key, int /*x*/, int /*y*/ )
		{
			get_instance( )->inner_keyboard( key );
		}

		inline void inner_keyboard( unsigned char key )
		{
			if ( keyboard_handler_ != 0 )
				keyboard_handler_->keyboard_handler( key );
		}

		static void thread( )
		{
			get_instance( )->inner_thread( );
		}

		inline void inner_thread( )
		{
			// Initialise the glut environment
			int fake_argc = 1;
			char* fake_argv[ ] = { "" };
			glutInit( &fake_argc, fake_argv );
			glutInitWindowSize( WIN_WIDTH, WIN_HEIGHT );
			glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGBA );
			glutCreateWindow( "OpenMediaLib GL/GLEW/GLUT output plugin" );
			glutDisplayFunc( display );
			glutTimerFunc( duration, timer, 0 );
			glutReshapeFunc( reshape );
			glutKeyboardFunc( keyboard );

			// Initialise the glew environment
			glewInit( );

			frame_type_ptr frame = get_frame( );

			if ( frame->get_image( ) != 0 )
			{
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
			}

			glutMainLoop( );
		}

		virtual bool push( frame_type_ptr frame )
		{
			if ( frame != 0 && audio_ == 0 )
			{
				audio_ = ml::create_store( L"openal:", frame );
				audio_->properties( ).get_property_with_string( "preroll" ) = 3;
				audio_->init( );
			}

			if ( frames_.size( ) > 10 )
				wait( );

			{
				boost::mutex::scoped_lock scoped_lock( mutex_ );
				frames_.push_back( frame );
				notify( );
			}
			return true;
		}

		virtual frame_type_ptr flush( )
		{
			{
				boost::mutex::scoped_lock scoped_lock( mutex_ );
				notify( );
			}

			if ( audio_ )
				audio_->flush( );

			{
				boost::mutex::scoped_lock scoped_lock( mutex_ );
				frame_type_ptr frame = frame_type_ptr( );
				if ( frames_.size( ) != 0 )
					frame = *frames_.begin( );
				frames_.clear( );
				notify( );
				return frame;
			}
		}

		virtual void complete( )
		{
			while( frames_.size( ) > 0 )
				wait( );
			done_ = true;

			{
				boost::mutex::scoped_lock scoped_lock( mutex_ );
				notify( );
			}
		}

	private:
		int WIN_WIDTH;
		int WIN_HEIGHT;
		int duration;
		boost::mutex mutex_;
		static glew_store *instance_;
		boost::mutex cond_mutex_;
		boost::condition cond_;
		store_type_ptr audio_;
		std::deque< ml::frame_type_ptr > frames_;
		GLuint texture_id;
		ml::store_keyboard_handler *keyboard_handler_;
		bool done_;
};

glew_store *glew_store::instance_ = 0;

//
// Plugin object
//

class ML_PLUGIN_DECLSPEC glew_plugin : public openmedialib_plugin
{
public:
	virtual input_type_ptr input(  const ostd::wstring & )
	{
		return input_type_ptr( );
	}

	virtual store_type_ptr store( const ostd::wstring &, const frame_type_ptr &frame )
	{
		return store_type_ptr( glew_store::get_instance( frame ) );
	}
};

} } }

//
// Access methods for openpluginlib
//

extern "C"
{
	ML_PLUGIN_DECLSPEC bool openplugin_init( void )
	{
		return true;
	}

	ML_PLUGIN_DECLSPEC bool openplugin_uninit( void )
	{
		return true;
	}
	
	ML_PLUGIN_DECLSPEC bool openplugin_create_plugin( const char*, opl::openplugin** plug )
	{
		*plug = new plugin::glew_plugin;
		return true;
	}
	
	ML_PLUGIN_DECLSPEC void openplugin_destroy_plugin( opl::openplugin* plug )
	{ 
		delete static_cast< plugin::glew_plugin * >( plug ); 
	}
}





