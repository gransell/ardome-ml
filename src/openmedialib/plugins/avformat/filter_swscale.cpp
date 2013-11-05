// swcale - a rescaling filter based on ffmpeg's swscale library
//
// Copyright (C) 2012 VizRT
// Released under the LGPL.

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <opencorelib/cl/enforce_defines.hpp>

#include <boost/cstdint.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>

namespace pl = olib::openpluginlib;

namespace ml = olib::openmedialib::ml;
namespace cl = olib::opencorelib;

namespace olib { namespace openmedialib { namespace ml {

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
			, ro_ ( new ml::image::rescale_object( ) )
		{
			properties( ).append( prop_enable_ = 1 );
			properties( ).append( prop_pf_ = std::wstring( L"" ) );
			properties( ).append( prop_progressive_ = 0 );
			properties( ).append( prop_interp_ = (int)ml::image::BICUBIC_SAMPLING );
			properties( ).append( prop_width_ = -1 );
			properties( ).append( prop_height_ = -1 );
			properties( ).append( prop_sar_num_ = -1 );
			properties( ).append( prop_sar_den_ = -1 );
			properties( ).append( prop_mode_ = std::wstring( L"fill" ) );
		}

		virtual ~filter_swscale( )
		{
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		virtual const std::wstring get_uri( ) const { return std::wstring( L"swscale" ); }

		virtual const size_t slot_count( ) const { return 1; }

	protected:

		void do_fetch( frame_type_ptr &frame )
		{
			// Handle the last frame logic
			if ( last_frame_ && last_frame_->get_position( ) == get_position( ) )
			{
				frame = last_frame_;
			}
			else
			{
				// Fetch the image
				frame = fetch_from_slot( );

				// Check if we're enabled and the frame has an image
				if ( prop_enable_.value< int >( ) && frame && frame->has_image( ) )
				{
					// Immediately shallow copy the frame
					frame = frame->shallow( );

					// For aspect ratio conversions
					ml::image::geometry shape;

					// Deal with the properties now
					shape.field_order = prop_progressive_.value< int >( ) ? ml::image::progressive : frame->get_image( )->field_order( );
					
					ml::image::rescale_mode_map_type::const_iterator i_mode = 
						ml::image::rescale_mode_map.find( prop_mode_.value< std::wstring >( ) );
					if( i_mode != ml::image::rescale_mode_map.end( ) )
						shape.mode = i_mode->second;
					
					shape.interp = prop_interp_.value< int >( );
					
					ml::image::MLPixelFormat mlpf = ml::image::string_to_MLPF( cl::str_util::to_t_string( prop_pf_.value< std::wstring >( ) ) );
					if ( ml::image::ML_PIX_FMT_NONE != mlpf )
						shape.pf = mlpf;
					
					shape.width = prop_width_.value< int >( );
					shape.height = prop_height_.value< int >( );
					shape.sar_num = prop_sar_num_.value< int >( );
					shape.sar_den = prop_sar_den_.value< int >( );

					ml::image_type_ptr image = frame->get_image( );

					// Rescale the image
					image = ml::image::rescale_and_convert( ro_, image, shape );

					// Set the image on the frame
					frame->set_image( image );

					// Set the sar on the frame
					frame->set_sar( shape.sar_num, shape.sar_den );
				}
			}

			// Hold on to a shallow copy in case the same frame is requested again
			if ( frame )
				last_frame_ = frame->shallow( );
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
		ml::rescale_object_ptr ro_;
};

filter_type_ptr ML_PLUGIN_DECLSPEC create_swscale( const std::wstring & )
{
    return filter_type_ptr( new filter_swscale( ) );
}

} } }
