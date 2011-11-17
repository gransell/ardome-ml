// An SVG input based on librsvg.
//
// Copyright (C) 2007 Ardendo
// Released under the LGPL.
//
// #input:svg:
//
// Renders an SVG image from a file or an XML string to an RGBA image.
//
// Property usage:
//
// fps_num, fps_den : int, int
//		The frame rate of the frame produced. Defaults to 25:1 (PAL)
//
// out : int
//		The duration of the input (in number of frames). Defaults to 1.
//
// xml : string
//		An SVG XML string to render. Will only be used if the input is
//		opened without an input file.
//
// render_width, render_height : int
//		The dimensions of the image to be rendered. If set to 0 (default),
//		the dimensions from the SVG data itself is used. See also the
//		stretch, render_sar_num and render_sar_den parameters below.
//
// stretch : int
//		If 0 (default), the render_width and render_height dimensions
//		are adjusted to maintain the SVG source aspect ratio. If not 0,
//		the output image dimensions always correspond exactly to
//		the render_width and render_height properties.
//
// render_sar_num, render_sar_den : int
//		The desired sample aspect ratio of the output image. If stretch
//		is set to 0, this value is taken into account so that the image
//		produced has the correct aspect ratio when viewed with the
//		given sar.

#include <opencorelib/cl/core.hpp>
#include <opencorelib/cl/enforce_defines.hpp>
#include <opencorelib/cl/guard_define.hpp>

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openpluginlib/pl/utf8_utils.hpp>
#include <openpluginlib/pl/pcos/isubject.hpp>
#include <openpluginlib/pl/pcos/observer.hpp>

#include <iostream>
#include <boost/thread.hpp>

#include <librsvg/rsvg.h>

namespace ml = olib::openmedialib::ml;
namespace pl = olib::openpluginlib;
namespace il = olib::openimagelib::il;
namespace cl = olib::opencorelib;
namespace pcos = olib::openpluginlib::pcos;

namespace aml { namespace openmedialib { 

typedef boost::recursive_mutex::scoped_lock scoped_lock;
static boost::recursive_mutex librsvg_mutex_;

void olib_rsvg_init( )
{
	static bool rsvg_initted = false;
	if ( !rsvg_initted )
	{
		g_thread_init( 0 );
		rsvg_init( );
		atexit( rsvg_term );
		rsvg_initted = true;
	}
}

class rsvg_observer : public pcos::observer
{
public:

	rsvg_observer( bool *dirty_flag )
		: dirty_flag_( dirty_flag )
	{}

	virtual ~rsvg_observer()
	{}

	virtual void updated( pcos::isubject* )
	{
		*dirty_flag_ = true;
	}

private:
	bool *dirty_flag_;
};

class ML_PLUGIN_DECLSPEC input_librsvg : public ml::input_type
{
	public:
		input_librsvg( const pl::wstring &resource ) 
			: ml::input_type( )
			, prop_resource_( pcos::key::from_string( "resource" ) )
			, dirty_( true )
			, prop_fps_num_( pcos::key::from_string( "fps_num" ) )
			, prop_fps_den_( pcos::key::from_string( "fps_den" ) )
			, prop_out_( pcos::key::from_string( "out" ) )
			, prop_xml_( pcos::key::from_string( "xml" ) )
			, prop_render_width_( pcos::key::from_string( "render_width" ) )
			, prop_render_height_( pcos::key::from_string( "render_height" ) )
			, prop_render_sar_num_( pcos::key::from_string( "render_sar_num" ) )
			, prop_render_sar_den_( pcos::key::from_string( "render_sar_den" ) )
			, prop_stretch_( pcos::key::from_string( "stretch" ) )
			, obs_dirty_state_( new rsvg_observer( &dirty_ ) )
			, frame_( )
		{
			properties( ).append( prop_resource_ = resource );
			properties( ).append( prop_fps_num_ = 25 );
			properties( ).append( prop_fps_den_ = 1 );
			properties( ).append( prop_out_ = 1 );
			properties( ).append( prop_xml_ = pl::wstring( L"" ) );

			//Set a custom width/height for the SVG rendering.
			//When set to 0, the default size given in the SVG data will be used.
			properties( ).append( prop_render_width_ = 0 );
			properties( ).append( prop_render_height_ = 0 );
			properties( ).append( prop_render_sar_num_ = 1 );
			properties( ).append( prop_render_sar_den_ = 1 );

			properties( ).append( prop_stretch_ = 0 );

			//Attach an observer which will set the dirty flag for
			//all properties that should cause us to re-render the SVG.
			prop_render_width_.attach( obs_dirty_state_ );
			prop_render_height_.attach( obs_dirty_state_ );
			prop_render_sar_num_.attach( obs_dirty_state_ );
			prop_render_sar_den_.attach( obs_dirty_state_ );
			prop_stretch_.attach( obs_dirty_state_ );
			prop_xml_.attach( obs_dirty_state_ );
			prop_resource_.attach( obs_dirty_state_ );
		}

		virtual ~input_librsvg( )
		{
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return true; }

		// Basic information
		virtual const pl::wstring get_uri( ) const { return prop_resource_.value< pl::wstring >( ); }
		virtual const pl::wstring get_mime_type( ) const { return L"image/svg"; }

		// Audio/Visual
		virtual int get_frames( ) const { return prop_out_.value< int >( ); }
		virtual bool is_seekable( ) const { return true; }
		virtual int get_video_streams( ) const { return 1; }
		virtual int get_audio_streams( ) const { return 0; }
		virtual int get_audio_channels_in_stream( int stream ) const { assert("No audio streams in svg input!"); return 0; }
				
		// FIXME: Currently librsvg breaks in threaded use
		virtual bool is_thread_safe( ) { return false; }

	protected:
		
		//Called by librsvg before the SVG image is rendered.
		//Gives us a chance to set the dimensions of rendered SVG.
		static void resizer_func(gint *width, gint *height, gpointer user_data)
		{
			const input_librsvg *me = reinterpret_cast<const input_librsvg*>(user_data);

			int w = me->prop_render_width_.value< int >( );
			int h = me->prop_render_height_.value< int >( );

			if( w > 0 && h > 0 )
			{
				if( me->prop_stretch_.value<int>() )
				{
					//Stretching allowed; just force the dimensions given
					//from the properties.
					*width = w;
					*height = h;
				}
				else
				{
					//No stretching allowed; maintain original SVG aspect ratio
					//by letter/pillarboxing as necessary.

					//First, get the wanted sample aspect ratio. We will need to tell librsvg
					//to shrink the rendered width by this amount, so that the image
					//looks correct when shown with the sar applied.
					double sar_factor = 1.0;
					int sar_num = me->prop_render_sar_num_.value< int >( );
					int sar_den = me->prop_render_sar_den_.value< int >( );
					if( sar_num > 0 && sar_den > 0 )
					{
						sar_factor = static_cast< double >( sar_num ) / static_cast< double >( sar_den );
					}

					//Next, find how much we can expand the image in the X and Y
					//directions and take the least of the two. This ensures that
					//we get the maximum size possible while still maintaining
					//the aspect ratio.
					double sizing_factor = std::min< double >( 
						static_cast< double >( w ) * sar_factor / (*width),
						static_cast< double >( h ) / (*height)
					);

					//Finally, set the new width/height, and adjust the width for sar.
					*width = static_cast< gint >( sizing_factor * (*width) / sar_factor + 0.5 );
					*height = static_cast< gint >( sizing_factor * (*height) + 0.5 );
				}
			}
		}

		//Helper function for cleaning up RSVG/GDK resources
		static void cleanup_rsvg( RsvgHandle **handle_ptr, GdkPixbuf **pixbuf_ptr )
		{
			scoped_lock lock( librsvg_mutex_ );

			if( *handle_ptr )
			{
				rsvg_handle_free( *handle_ptr );
				*handle_ptr = NULL;
			}

			if( *pixbuf_ptr )
			{
				g_object_unref( *pixbuf_ptr );
				*pixbuf_ptr = NULL;
			}
		}

		void do_fetch( ml::frame_type_ptr &result )
		{
			if( dirty_ )
			{
				RsvgHandle *handle = NULL;
				GdkPixbuf *pixbuf = NULL;
				//Exception-safe cleanup
				ARGUARD( boost::bind( &cleanup_rsvg, &handle, &pixbuf ) );

				GError *error = NULL;
				scoped_lock lock( librsvg_mutex_ );

				if ( prop_resource_.value< pl::wstring >( ).empty() || prop_resource_.value< pl::wstring >( ) == L"svg:" )
				{
					//No SVG file given as input, load SVG XML from the "xml" property instead
					std::string xml_str = pl::to_string( prop_xml_.value< pl::wstring >( ) );

					ARENFORCE_MSG( !xml_str.empty(), "SVG input: no file was given and the xml property is empty. Nothing to render." );
					
					handle = rsvg_handle_new( );
					ARENFORCE_MSG( handle, "SVG input: failed to create libsrvg handle: \"%1%\"" )( error->message );

					ARENFORCE_MSG( rsvg_handle_write( handle, reinterpret_cast< const guchar * >( xml_str.c_str( ) ), xml_str.size( ), &error ),
						"SVG input: error when loading SVG data from xml property: \"%1%\"" )( error->message );
				}
				else
				{
					//Remove the svg: prefix, if any
					std::string filename = pl::to_string( prop_resource_.value< pl::wstring >( ) );
					if( filename.find( "svg:" ) == 0 )
					{
						filename = filename.substr( 4 );
					}

					handle = rsvg_handle_new_from_file( filename.c_str(), &error );
					ARENFORCE_MSG( handle, "SVG input: Failed to create librsvg handle from file \"%1%\".\nError returned was: \"%2%\"" )
						( filename )( error->message );
				}

				//Close the handle, which blocks until the I/O operations are finished
				ARENFORCE_MSG( rsvg_handle_close( handle, &error ), "Error when closing librsvg handle: \"%1%\"" )( error->message );

				rsvg_handle_set_size_callback( handle, &resizer_func, this, 0);

				pixbuf = rsvg_handle_get_pixbuf( handle );
				ARENFORCE_MSG( pixbuf, "SVG input: Failed to create pixbuf from RSVG. Reason unknown." );

				boost::uint8_t *src = gdk_pixbuf_get_pixels( pixbuf );
				int src_pitch = gdk_pixbuf_get_rowstride( pixbuf );
				int w = gdk_pixbuf_get_width( pixbuf );
				int h = gdk_pixbuf_get_height( pixbuf );
				int bytes = 0;

				w -= w % 2;
				h -= h % 2;

				il::image_type_ptr image;

				if ( w > 0 && h > 0 )
				{
					if ( gdk_pixbuf_get_has_alpha( pixbuf ) )
					{
						image = il::allocate( L"r8g8b8a8", w, h );
						bytes = w * 4;
					}
					else
					{
						image = il::allocate( L"r8g8b8", w, h );
						bytes = w * 3;
					}
	
					boost::uint8_t *dst = image->data( );
					int dst_pitch = image->pitch( );
	
					while( h -- )
					{
						memcpy( dst, src, bytes );
						dst += dst_pitch;
						src += src_pitch;
					}
				}

				frame_ = ml::frame_type_ptr( new ml::frame_type( ) );

				int sar_num = prop_render_sar_num_.value< int >( );
				int sar_den = prop_render_sar_den_.value< int >( );
				if( sar_num > 0 && sar_den > 0 )
				{
					frame_->set_sar( sar_num, sar_den );
					image->set_sar_num( sar_num );
					image->set_sar_den( sar_den );
				}
				frame_->set_image( image );

				//Our image is now rendered and in sync with the current state
				dirty_ = false;
			}

			ARENFORCE( frame_ );

			result = frame_->shallow( );
			result->set_position( get_position( ) );
			result->set_fps( prop_fps_num_.value< int >( ), prop_fps_den_.value< int >( ) );
		}

	protected:

		pcos::property prop_resource_;
		bool dirty_; //Whether the SVG needs to be re-rendered on the next fetch
		pcos::property prop_fps_num_;
		pcos::property prop_fps_den_;
		pcos::property prop_out_;
		pcos::property prop_xml_;
		pcos::property prop_render_width_;
		pcos::property prop_render_height_;
		pcos::property prop_render_sar_num_;
		pcos::property prop_render_sar_den_;
		pcos::property prop_stretch_;
		boost::shared_ptr< rsvg_observer > obs_dirty_state_;
		ml::frame_type_ptr frame_;
};

ml::input_type_ptr ML_PLUGIN_DECLSPEC create_input_librsvg( const pl::wstring &resource )
{
	return ml::input_type_ptr( new input_librsvg( resource ) );
}

} }

