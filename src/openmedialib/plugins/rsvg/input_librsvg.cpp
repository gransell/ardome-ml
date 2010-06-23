// An SVG reader
//
// Copyright (C) 2007 Ardendo
// Released under the LGPL.
//
// #input:svg:
//
// An SVG input based on librsvg.

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
static boost::recursive_mutex mutex_;

void olib_rsvg_init( )
{
	static bool rsvg_initted = false;
	if ( !rsvg_initted )
	{
		rsvg_initted = true;
		rsvg_init( );
		atexit( rsvg_term );
	}
}

class ML_PLUGIN_DECLSPEC input_librsvg : public ml::input_type
{
	public:
		input_librsvg( const pl::wstring &resource ) 
			: ml::input_type( )
			, prop_resource_( pcos::key::from_string( "resource" ) )
			, loaded_( L"" )
			, prop_fps_num_( pcos::key::from_string( "fps_num" ) )
			, prop_fps_den_( pcos::key::from_string( "fps_den" ) )
			, prop_out_( pcos::key::from_string( "out" ) )
			, prop_doc_( pcos::key::from_string( "doc" ) )
			, prop_deferred_( pcos::key::from_string( "deferred" ) )
			, prop_render_width_( pcos::key::from_string( "render_width" ) )
			, prop_render_height_( pcos::key::from_string( "render_height" ) )
			, frame_( )
		{
			properties( ).append( prop_resource_ = resource );
			properties( ).append( prop_fps_num_ = 1 );
			properties( ).append( prop_fps_den_ = 1 );
			properties( ).append( prop_out_ = 1 );
			properties( ).append( prop_doc_ = pl::wstring( L"" ) );
			properties( ).append( prop_deferred_ = 0 );

			//Set a custom width/height for the SVG rendering.
			//When set to 0, the default size given in the SVG data will be used.
			properties( ).append( prop_render_width_ = 0);
			properties( ).append( prop_render_height_ = 0);
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

	protected:
		bool matches_deferred( ml::frame_type_ptr &frame )
		{
			if ( frame && frame->get_image( ) )
			{
				if ( prop_deferred_.value< int >( ) == 0 )
					return il::is_yuv_planar( frame->get_image( ) );
				else
					return !il::is_yuv_planar( frame->get_image( ) );
			}
			return false;
		}
		
		//Called by librsvg before the SVG image is rendered.
		//Gives us a chance to set the dimensions of rendered SVG.
		static void resizerFunc(gint *width, gint *height, gpointer user_data)
		{
			const input_librsvg *me = reinterpret_cast<const input_librsvg*>(user_data);
			
			if (me->prop_render_width_.value<int>() > 0)
			{
				*width = me->prop_render_width_.value<int>();
			}
			if (me->prop_render_height_.value<int>() > 0)
			{
				*height = me->prop_render_height_.value<int>();
			}
		}

		void do_fetch( ml::frame_type_ptr &result )
		{
			// Invite a callback here
			acquire_values( );

			GdkPixbuf *pixbuf = NULL;

			if ( prop_resource_.value< pl::wstring >( ) == L"svg:" )
			{
				scoped_lock lock( mutex_ );

				std::string doc = pl::to_string( prop_doc_.value< pl::wstring >( ) );

				if ( doc != "" && ( prop_doc_.value< pl::wstring >( ) != loaded_ || !matches_deferred( frame_ ) ) )
				{
					RsvgHandle *handle = rsvg_handle_new( );
					GError *error = NULL;

					rsvg_handle_set_size_callback( handle, &resizerFunc, this, 0);

					if ( rsvg_handle_write( handle, reinterpret_cast< const guchar * >( doc.c_str( ) ), 
											doc.size( ), &error ) )
					{
						if ( rsvg_handle_close( handle, &error ) )
						{
							pixbuf = rsvg_handle_get_pixbuf( handle );
							rsvg_handle_free( handle );
						}
						else
						{
						}
					}
					else
					{
					}

					loaded_ = prop_doc_.value< pl::wstring >( );
				}
			}
			else if ( prop_resource_.value< pl::wstring >( ) != loaded_ )
			{
				loaded_ = prop_resource_.value< pl::wstring >( );
				pixbuf = rsvg_pixbuf_from_file( pl::to_string( loaded_ ).c_str( ), NULL );
			}

			if ( pixbuf )
			{
				scoped_lock lock( mutex_ );
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
				frame_->set_image( image );

				if ( prop_deferred_.value< int >( ) == 0 )
					frame_ = ml::frame_convert( frame_, L"yuv420p" );

				g_object_unref( pixbuf );
			}

			if ( frame_ )
			{
				result = frame_->deep( );
				result->set_position( get_position( ) );
				result->set_sar( 1, 1 );
				result->set_fps( prop_fps_num_.value< int >( ), prop_fps_den_.value< int >( ) );
			}
		}

	protected:

		pcos::property prop_resource_;
		pl::wstring loaded_;
		pcos::property prop_fps_num_;
		pcos::property prop_fps_den_;
		pcos::property prop_out_;
		pcos::property prop_doc_;
		pcos::property prop_deferred_;
		pcos::property prop_render_width_;
		pcos::property prop_render_height_;
		ml::frame_type_ptr frame_;
};

ml::input_type_ptr ML_PLUGIN_DECLSPEC create_input_librsvg( const pl::wstring &resource )
{
	return ml::input_type_ptr( new input_librsvg( resource ) );
}

} }

