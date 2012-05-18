// ml - A media library representation.

// Copyright (C) 2011 Vizrt
// Released under the LGPL.

#ifndef ML_ANALYSE_DNXHD
#define ML_ANALYSE_DNXHD

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/analyse.hpp>

namespace olib { namespace openmedialib { namespace ml {

class ML_DECLSPEC analyse_dnxhd : public analyse_type
{
	public:
		analyse_dnxhd( );

		virtual ~analyse_dnxhd( ) { }

		bool valid( const std::string &codec );

		bool analysis( boost::uint8_t *data, const size_t size );

		bool collect( olib::openpluginlib::pcos::property_container &properties );

		int cid( boost::uint8_t *data, const size_t size );
		int framesize( int cid );
		int sample_bit_depth( int cid );
		bool interlace( int cid );
		int height( int cid );
		int width( int cid );
		int bitrate( int cid, const int fps_num, const int fps_den );

	private:
		int id_;
		olib::openimagelib::il::field_order_flags field_;
};

} } }

#endif
