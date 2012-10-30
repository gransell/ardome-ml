// swcale - a rescaling filter based on ffmpeg's swscale library
//
// Copyright (C) 2012 VizRT
// Released under the LGPL.

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <opencorelib/cl/enforce_defines.hpp>

#include <boost/cstdint.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

namespace plugin = olib::openmedialib::ml;
namespace pl = olib::openpluginlib;
namespace il = olib::openimagelib::il;
namespace ml = olib::openmedialib::ml;

namespace olib { namespace openmedialib { namespace ml {

extern const PixelFormat oil_to_avformat( const std::wstring & );

namespace 
{
	static void correct( int mw, int mh, int &w, int &h )
	{
		int wd = w % mw;
		int hd = h % mh;
		if ( wd != 0 )
			w += mw - wd;
		if ( hd != 0 )
			h += mh - hd;
	}

	typedef boost::function< void ( int &, int & ) > correct_function;

	typedef std::map< std::wstring, correct_function > corrections_map;

	static corrections_map create_corrections( )
	{
		corrections_map result;

		result[ std::wstring( L"b8g8r8" ) ] = boost::bind( correct, 1, 1, _1, _2 );
		result[ std::wstring( L"b8g8r8a8" ) ] = boost::bind( correct, 1, 1, _1, _2 );
		result[ std::wstring( L"r8g8b8" ) ] = boost::bind( correct, 1, 1, _1, _2 );
		result[ std::wstring( L"r8g8b8a8" ) ] = boost::bind( correct, 1, 1, _1, _2 );
		result[ std::wstring( L"yuv411p" ) ] = boost::bind( correct, 4, 1, _1, _2 );
		result[ std::wstring( L"yuv420p" ) ] = boost::bind( correct, 2, 2, _1, _2 );
		result[ std::wstring( L"yuv422p" ) ] = boost::bind( correct, 2, 1, _1, _2 );
		result[ std::wstring( L"yuv422" ) ] = boost::bind( correct, 2, 1, _1, _2 );
		result[ std::wstring( L"uyv422" ) ] = boost::bind( correct, 2, 1, _1, _2 );

		return result;
	}

	static const corrections_map corrections = create_corrections( );

	static void correct( const std::wstring &pf, int &w, int &h )
	{
		corrections_map::const_iterator iter = corrections.find( pf );
		if ( iter != corrections.end( ) )
			iter->second( w, h );
	}
}

class ML_PLUGIN_DECLSPEC filter_swscale : public filter_simple
{
	public:
		filter_swscale( )
			: filter_simple( )
			, prop_enable_( pl::pcos::key::from_string( "enable" ) )
			, prop_pf_( pl::pcos::key::from_string( "pf" ) )
			, prop_progressive_( pl::pcos::key::from_string( "progressive" ) )
			, prop_interp_( pl::pcos::key::from_string( "interp" ) )
			, prop_width_( pl::pcos::key::from_string( "width" ) )
			, prop_height_( pl::pcos::key::from_string( "height" ) )
			, prop_sar_num_( pl::pcos::key::from_string( "sar_num" ) )
			, prop_sar_den_( pl::pcos::key::from_string( "sar_den" ) )
			, scaler_( 0 )
		{
			properties( ).append( prop_enable_ = 1 );
			properties( ).append( prop_pf_ = std::wstring( L"" ) );
			properties( ).append( prop_progressive_ = 0 );
			properties( ).append( prop_interp_ = 4 );
			properties( ).append( prop_width_ = -1 );
			properties( ).append( prop_height_ = -1 );
			properties( ).append( prop_sar_num_ = -1 );
			properties( ).append( prop_sar_den_ = -1 );
		}

		virtual ~filter_swscale( )
		{
			if ( scaler_ )
				sws_freeContext( scaler_ );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		virtual const std::wstring get_uri( ) const { return std::wstring( L"swscale" ); }

		virtual const size_t slot_count( ) const { return 1; }

	protected:

		void do_fetch( frame_type_ptr &frame )
		{
			if ( last_frame_ && last_frame_->get_position( ) == get_position( ) )
			{
				frame = last_frame_->shallow( );
			}
			else
			{
				frame = fetch_from_slot( );
				if ( prop_enable_.value< int >( ) && frame && frame->has_image( ) )
				{
					il::image_type_ptr image = frame->get_image( );

					// Force progressive or lie :-/
					if ( prop_progressive_.value< int >( ) == 1 )
						image = il::deinterlace( image );
					else if ( prop_progressive_.value< int >( ) == -1 )
						image->set_field_order( il::progressive );

					// Determine the current field order
					il::field_order_flags field_order = image->field_order( );

					// Deal with the properties now
					int width = prop_width_.value< int >( );
					int height = prop_height_.value< int >( );
					std::wstring pf = prop_pf_.value< std::wstring >( );
					int interp = prop_interp_.value< int >( );
					int sar_num = prop_sar_num_.value< int >( );
					int sar_den = prop_sar_den_.value< int >( );

					// Deal with the properties specified
					// TODO: Sanitise requested values
					if ( width == -1 && height == -1 )
					{
						// If width and height are -1, then retain the dimensions of the source
						width = image->width( );
						height = image->height( );
					}
					else if ( width == -1 && std::abs( sar_num ) == 1 && std::abs( sar_den ) == 1 )
					{
						// Maintain the aspect ratio of the input, and calculate width from height
						width = int( height * frame->aspect_ratio( ) );
						sar_num = 1;
						sar_den = 1;
					}
					else if ( height == -1 && std::abs( sar_num ) == 1 && std::abs( sar_den ) == 1 )
					{
						// Maintain the aspect ratio of the input, and calculate height from width
						height = int( width / frame->aspect_ratio( ) );
						sar_num = 1;
						sar_den = 1;
					}
					else
					{
						// TODO: Other combinations
					}

					// If no conversion is specified, retain that of the input
					if ( pf == L"" )
					{
						pf = image->pf( );
					}

					// Ensure that the requested/computed dimensions are valid for the pf requested
					correct( pf, width, height );

					// If no sar is specified, retain it from the source
					if ( sar_num == -1 || sar_den == -1 )
					{
						frame->get_sar( sar_num, sar_den );
					}

					// Rescale the image
					image = rescale( image, pf, width, height, interp );

					// Set the original field order
					image->set_field_order( field_order );

					// Set the image on the frame
					frame->set_image( image );

					// Set the sar on the frame
					frame->set_sar( sar_num, sar_den );
				}
				last_frame_ = frame->shallow( );
			}
		}

		il::image_type_ptr rescale( const il::image_type_ptr &input, const std::wstring &pf, int width, int height, int filter )
		{
			// Allocate the output image
			il::image_type_ptr output = il::allocate( pf, width, height );

			// Determine the libswscale pix format values
			PixelFormat in_format = oil_to_avformat( input->pf( ) );
			ARENFORCE_MSG( in_format != PIX_FMT_NONE, "Don't know how to map pf %s to pixfmt" )( input->pf( ) );

			PixelFormat out_format = oil_to_avformat( pf );
			ARENFORCE_MSG( out_format != PIX_FMT_NONE, "Don't know how to map pf %s to pixfmt" )( pf );

			// Fill in the input picture
			// FIXME: Should take the pitch into account
			AVPicture in;
			avpicture_fill( &in, input->data( ), in_format, input->width( ), input->height( ) );

			// Fill in the output picture
			// FIXME: Should take the pitch into account
			AVPicture out;
			avpicture_fill( &out, output->data( ), out_format, output->width( ), output->height( ) );

			// Obtain the context for the scaling (probably as close to a no op as you get unless input changes)
			scaler_ = sws_getCachedContext( scaler_, input->width( ), input->height( ), in_format, output->width( ), output->height( ), out_format, filter, NULL, NULL, NULL );

			// Ensure that the conversion is valid
			ARENFORCE_MSG( scaler_, "Unable to convert from %s to %s" )( input->pf( ) )( pf );

			// Scale the image
			sws_scale( scaler_, in.data, in.linesize, 0, input->height( ), out.data, out.linesize );

			return output;
		}

	private:
		pl::pcos::property prop_enable_;
		pl::pcos::property prop_pf_;
		pl::pcos::property prop_progressive_;
		pl::pcos::property prop_interp_;
		pl::pcos::property prop_width_;
		pl::pcos::property prop_height_;
		pl::pcos::property prop_sar_num_;
		pl::pcos::property prop_sar_den_;
		ml::frame_type_ptr last_frame_;
		struct SwsContext *scaler_;
};

filter_type_ptr ML_PLUGIN_DECLSPEC create_swscale( const std::wstring & )
{
    return filter_type_ptr( new filter_swscale( ) );
}

} } }
