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

namespace pl = olib::openpluginlib;
namespace il = olib::openimagelib::il;
namespace ml = olib::openmedialib::ml;
namespace cl = olib::opencorelib;

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
		result[ std::wstring( L"yuv422" ) ] = boost::bind( correct, 4, 1, _1, _2 );
		result[ std::wstring( L"uyv422" ) ] = boost::bind( correct, 4, 1, _1, _2 );

		return result;
	}

	static const corrections_map corrections = create_corrections( );

	static void correct( const std::wstring &pf, int &w, int &h )
	{
		corrections_map::const_iterator iter = corrections.find( pf );
		if ( iter != corrections.end( ) )
			iter->second( w, h );
	}

	struct geometry 
	{
		geometry( )
		: cropped( false )
		, pf( L"" )
		, width( 0 )
		, height( 0 )
		, sar_num( 0 )
		, sar_den( 0 )
		, x( 0 )
		, y( 0 )
		, w( 0 )
		, h( 0 )
		, cx( 0 )
		, cy( 0 )
		, cw( 0 )
		, ch( 0 )
		{ }

		// Indicates if the input image should be cropped
		bool cropped; 

		// Specifies the picture format of the full image
		std::wstring pf;

		// Specifies the dimensions of the full image
		int width, height, sar_num, sar_den;

		// Specifies the geometry of the scaled image
		int x, y, w, h;

		// Specifes the geometry of the crop
		int cx, cy, cw, ch; 
	};

	// Fill an AVPicture with a potentially cropped aml image
	AVPicture fill_picture( il::image_type_ptr image )
	{
		AVPicture picture;

		for ( int i = 0; i < AV_NUM_DATA_POINTERS; i ++ )
		{
			if ( i < image->plane_count( ) )
			{
				picture.data[ i ] = image->data( i );
				picture.linesize[ i ] = image->pitch( i );
			}
			else
			{
				picture.data[ i ] = 0;
				picture.linesize[ i ] = 0;
			}
		}

		return picture;
	}

	// Calculate the geometry for the aspect ratio mode selected
	void calculate( struct geometry &shape, frame_type_ptr src, const std::wstring &mode )
	{
		shape.cropped = true;

		double src_sar_num = double( src->get_sar_num( ) ); 
		double src_sar_den = double( src->get_sar_den( ) );
		double dst_sar_num = double( shape.sar_num );
		double dst_sar_den = double( shape.sar_den );

		int src_w = src->get_image( )->width( );
		int src_h = src->get_image( )->height( );
		int dst_w = shape.width;
		int dst_h = shape.height;

		if ( src_sar_num <= 0 ) src_sar_num = 1;
		if ( src_sar_den <= 0 ) src_sar_den = 1;
		if ( dst_sar_num <= 0 ) dst_sar_num = 1;
		if ( dst_sar_den <= 0 ) dst_sar_den = 1;

		// Distort image to fill
		shape.x = 0;
		shape.y = 0;
		shape.w = dst_w;
		shape.h = dst_h;

		// Specify the initial crop area
		shape.cx = 0;
		shape.cy = 0;
		shape.cw = src_w;
		shape.ch = src_h;

		// Target destination area
		dst_w = shape.w;
		dst_h = shape.h;

		// Letter and pillar box calculations
		int letter_h = int( 0.5 + ( shape.w * src_h * src_sar_den * dst_sar_num ) / ( src_w * src_sar_num * dst_sar_den ) );
		int pillar_w = int( 0.5 + ( shape.h * src_w * src_sar_num * dst_sar_den ) / ( src_h * src_sar_den * dst_sar_num ) );

		correct( shape.pf, pillar_w, letter_h );

		// Handle the requested mode
		if ( mode == L"fill" )
		{
			shape.h = dst_h;
			shape.w = pillar_w;

			if ( shape.w > dst_w )
			{
				shape.w = dst_w;
				shape.h = letter_h;
			}
		}
		else if ( mode == L"smart" )
		{
			shape.h = dst_h;
			shape.w = pillar_w;

			if ( shape.w < dst_w )
			{
				shape.w = dst_w;
				shape.h = letter_h;
			}
		}
		else if ( mode.find( L"letter" ) == 0 )
		{
			shape.w = dst_w;
			shape.h = letter_h;
		}
		else if ( mode.find( L"pillar" ) == 0 )
		{
			shape.h = dst_h;
			shape.w = pillar_w;
		}
		else if ( mode.find( L"native" ) == 0 )
		{
			shape.w = src_w;
			shape.h = src_h;
		}

		// Correct the cropping as required
		if ( dst_w >= shape.w )
		{
			shape.x += ( int( dst_w ) - shape.w ) / 2;
		}
		else
		{
			double diff = double( shape.w - dst_w ) / shape.w;
			shape.cx = int( shape.cw * ( diff / 2.0 ) );
			shape.cw = int( shape.cw * ( 1.0 - diff ) );
			shape.w = dst_w;
		}

		if ( dst_h >= shape.h )
		{
			shape.y += ( int( dst_h ) - shape.h ) / 2;
		}
		else
		{
			double diff = double( shape.h - dst_h ) / shape.h;
			shape.cy = int( shape.ch * ( diff / 2.0 ) );
			shape.ch = int( shape.ch * ( 1.0 - diff ) );
			shape.h = dst_h;
		}

		correct( src->get_image( )->pf( ), shape.cx, shape.cy );
		correct( src->get_image( )->pf( ), shape.cw, shape.ch );
		correct( shape.pf, shape.x, shape.y );
		correct( shape.pf, shape.w, shape.h );
	}

	inline void colour_rectangle( boost::uint8_t *ptr, boost::uint8_t value, int width, int pitch, int height )
	{
		while( height -- > 0 )
		{
			memset( ptr, value, width );
			ptr += pitch;
		}
	}

	// Draw the necessary border around the cropped image
	// TODO: Support picture formats other than yuv420p
	void border( il::image_type_ptr image, const geometry &shape, int r = 0, int g = 0, int b = 0 )
	{
		int yuv[ 3 ];
		il::rgb24_to_yuv444( yuv[ 0 ], yuv[ 1 ], yuv[ 2 ], r, g, b );

		for ( int i = 0; i < image->plane_count( ); i ++ )
		{
			const float wps = float( image->linesize( i ) ) / image->width( 0 );
			const float hps = float( image->height( i ) ) / image->height( 0 );

			boost::uint8_t *ptr = image->data( i );
			const boost::uint8_t value = yuv[ i ];

			const int width = image->width( i );
			const int height = image->height( i );
			const int pitch = image->pitch( i );
			const int top_lines = int( shape.y * hps );
			const int bottom_lines = height - int( ( shape.y + shape.h ) * hps );
			const int bottom_offset = int( pitch * ( shape.y + shape.h ) * hps );
			const int side_lines = int( shape.h * hps );
			const int left_offset = int( pitch * shape.y * hps );
			const int right_offset = left_offset + int( ( shape.x + shape.w ) * wps );
			const int left_width = int( shape.x * wps );
			const int right_width = int( width - ( shape.x + shape.w ) * wps );

			//std::cerr << left_offset << " + " << left_width << " to " << right_offset << " + " << right_width << " " << shape.w << std::endl;

			colour_rectangle( ptr, value, width, pitch, top_lines );
			colour_rectangle( ptr + left_offset, value, left_width, pitch, side_lines );
			colour_rectangle( ptr + right_offset, value, right_width, pitch, side_lines );
			colour_rectangle( ptr + bottom_offset, value, width, pitch, bottom_lines );
		}
	}

	il::image_type_ptr rescale( struct SwsContext *&scaler_, const il::image_type_ptr &input, int filter, const struct geometry &shape )
	{
		// Allocate the output image
		il::image_type_ptr output = il::allocate( shape.pf, shape.width, shape.height );

		// Determine the libswscale pix format values
		PixelFormat in_format = oil_to_avformat( input->pf( ) );
		ARENFORCE_MSG( in_format != PIX_FMT_NONE, "Don't know how to map pf %s to pixfmt" )( input->pf( ) );

		PixelFormat out_format = oil_to_avformat( shape.pf );
		ARENFORCE_MSG( out_format != PIX_FMT_NONE, "Don't know how to map pf %s to pixfmt" )( shape.pf );

		// Crop the input if the shape is cropped
		if ( shape.cropped )
			input->crop( shape.cx, shape.cy, shape.cw, shape.ch );

		// Fill in the input picture
		AVPicture in = fill_picture( input );

		// Crop the output if the shape is cropped
		if ( shape.cropped )
		{
			border( output, shape, 0, 0, 0 );
			output->crop( shape.x, shape.y, shape.w, shape.h );
		}

		// Fill in the output picture
		AVPicture out = fill_picture( output );

		// Obtain the context for the scaling (probably as close to a no op as you get unless input changes)
		scaler_ = sws_getCachedContext( scaler_, input->width( ), input->height( ), in_format, output->width( ), output->height( ), out_format, filter, NULL, NULL, NULL );

		// Ensure that the conversion is valid
		ARENFORCE_MSG( scaler_, "Unable to convert from %s to %s" )( input->pf( ) )( shape.pf );

		// Scale the image
		sws_scale( scaler_, in.data, in.linesize, 0, input->height( ), out.data, out.linesize );

		// Clear crop information on both images if shape is cropped
		if ( shape.cropped )
		{
			input->crop_clear( );
			output->crop_clear( );
		}

		return output;
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
			, prop_mode_( pl::pcos::key::from_string( "mode" ) )
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
			properties( ).append( prop_mode_ = std::wstring( L"fill" ) );
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

					// For aspect ratio conversions
					struct geometry shape;

					// Deal with the properties now
					std::wstring mode = prop_mode_.value< std::wstring >( );
					int interp = prop_interp_.value< int >( );

					// Default the values of the shape
					shape.pf = prop_pf_.value< std::wstring >( );
					shape.width = prop_width_.value< int >( );
					shape.height = prop_height_.value< int >( );
					shape.sar_num = prop_sar_num_.value< int >( );
					shape.sar_den = prop_sar_den_.value< int >( );

					// If no conversion is specified, retain that of the input
					if ( shape.pf == L"" )
						shape.pf = image->pf( );

					// Deal with the properties specified
					if ( shape.width == -1 && shape.height == -1 && std::abs( shape.sar_num ) == 1 && std::abs( shape.sar_den ) == 1 )
					{
						// If width and height are -1, then retain the dimensions of the source
						shape.width = image->width( );
						shape.height = image->height( );
						shape.sar_num = 1;
						shape.sar_den = 1;
					}
					else if ( shape.width == -1 && std::abs( shape.sar_num ) == 1 && std::abs( shape.sar_den ) == 1 )
					{
						// Maintain the aspect ratio of the input, and calculate width from height
						shape.width = int( shape.height * frame->aspect_ratio( ) );
						shape.sar_num = 1;
						shape.sar_den = 1;
					}
					else if ( shape.height == -1 && std::abs( shape.sar_num ) == 1 && std::abs( shape.sar_den ) == 1 )
					{
						// Maintain the aspect ratio of the input, and calculate height from width
						shape.height = int( shape.width / frame->aspect_ratio( ) );
						shape.sar_num = 1;
						shape.sar_den = 1;
					}
					else if ( shape.sar_num != -1 && shape.sar_num != -1 )
					{
						calculate( shape, frame, mode );
					}

					// Ensure that the requested/computed dimensions are valid for the pf requested
					correct( shape.pf, shape.width, shape.height );

					// If no sar is specified, retain it from the source
					if ( shape.sar_num == -1 || shape.sar_den == -1 )
					{
						frame->get_sar( shape.sar_num, shape.sar_den );
					}

					// Rescale the image
					image = rescale( scaler_, image, interp, shape );

					// Set the original field order
					image->set_field_order( field_order );

					// Set the image on the frame
					frame->set_image( image );

					// Set the sar on the frame
					frame->set_sar( shape.sar_num, shape.sar_den );
				}

				last_frame_ = frame->shallow( );
			}
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
		pl::pcos::property prop_mode_;
		ml::frame_type_ptr last_frame_;
		struct SwsContext *scaler_;
};

filter_type_ptr ML_PLUGIN_DECLSPEC create_swscale( const std::wstring & )
{
    return filter_type_ptr( new filter_swscale( ) );
}

} } }
