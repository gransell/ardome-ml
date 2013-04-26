// ml - A media library representation.

// Copyright (C) 2011 Vizrt
// Released under the LGPL.

#include "analyse_dv.hpp"
#include <openmedialib/ml/image/image.hpp>

namespace pl = olib::openpluginlib;

namespace olib { namespace openmedialib { namespace ml {

typedef boost::rational< boost::int64_t > rational;

typedef struct dv_type
{
	int dsf;
	int stype;
	int fps_num;
	int fps_den;
	size_t size;
	int width;
	int height;
	olib::t_string pf;
	ml::image::field_order_flags field;
}
dv_type;

static const dv_type dv_profiles[ ] =
{
	{ 0,  0x0, 30000, 1001, 120000,  720,  480, "yuv411p", ml::image::bottom_field_first },	// DV25 NTSC
	{ 1,  0x0,    25,    1, 144000,  720,  576, "yuv420p", ml::image::bottom_field_first },	// DV25 PAL
	{ 1,  0x0,    25,    1, 144000,  720,  576, "yuv411p", ml::image::bottom_field_first },	// DVCPRO
	{ 0,  0x4, 30000, 1001, 240000,  720,  480, "yuv422p", ml::image::bottom_field_first },	// DVCPRO50 NTSC
	{ 1,  0x4,    25,    1, 288000,  720,  576, "yuv422p", ml::image::bottom_field_first },	// DVCPRO50 PAL
	{ 0, 0x14, 30000, 1001, 480000, 1280, 1080, "yuv422p", ml::image::top_field_first	   },	// DV100 HD NTSC
	{ 1, 0x14,    25,    1, 576000, 1440, 1080, "yuv422p", ml::image::top_field_first	   },	// DV100 HD PAL
	{ 0, 0x18, 60000, 1001, 240000,  960,  720, "yuv422p", ml::image::progressive		   },	// DVCPRO HD NTSC
	{ 1, 0x18,    50,    1, 288000,  960,  720, "yuv422p", ml::image::progressive		   },	// DVCPRO HD PAL
	{ 1,  0x1,    25,    1, 144000,  720,  576, "yuv420p", ml::image::bottom_field_first },	// DV25 PAL?
	{ -1, 0x0,     1,    1,      0,    0,    0,        "", ml::image::progressive		   }
};

analyse_dv::analyse_dv( )
: id_( -1 )
, is_wide_( false )
{
}

bool analyse_dv::valid( const std::string &codec )
{
	return true;
}

bool analyse_dv::analysis( boost::uint8_t *data, const size_t size )
{
	id_ = id( data, size );

	bool result = id_ != -1 && dv_profiles[ id_ ].size % size == 0;

	if ( result )
	{
		boost::uint8_t *vsc_pack = data + 80 * 5 + 48 + 5;
		int apt = get_bits( data, 0x32, 8 ) & 0x07;
		if ( *vsc_pack == 0x61 )
			is_wide_ = ( vsc_pack[ 2 ] & 0x07 ) == 0x02 || ( !apt && ( vsc_pack[ 2 ] & 0x07 ) == 0x07 );
		else
			is_wide_ = true;
	}

	return result;
}

bool analyse_dv::collect( pl::pcos::property_container &properties )
{
	bool result = id_ != -1;

	if ( result )
	{
		const dv_type &dv = dv_profiles[ id_ ];
		int display_width = int( 3.0 * ( is_wide_ ? dv.height * 16.0 / 9 : dv.height * 4.0 / 3 ) + 0.5 );
		rational sar( display_width, dv.width * 3 );
		properties.append( pl::pcos::property( key_fps_num_ ) = dv.fps_num );
		properties.append( pl::pcos::property( key_fps_den_ ) = dv.fps_den );
		properties.append( pl::pcos::property( key_packet_size_ ) = int( dv.size ) );
		properties.append( pl::pcos::property( key_width_ ) = dv.width );
		properties.append( pl::pcos::property( key_height_ ) = dv.height );
		properties.append( pl::pcos::property( key_pf_ ) = dv.pf );
		properties.append( pl::pcos::property( key_field_order_ ) = int( dv.field ) );
		properties.append( pl::pcos::property( key_sar_num_ ) = int( sar.numerator( ) ) );
		properties.append( pl::pcos::property( key_sar_den_ ) = int( sar.denominator( ) ) );
		properties.append( pl::pcos::property( key_picture_coding_type_ ) = 1 );
	}

	return result;
}

int analyse_dv::id( boost::uint8_t *data, const size_t size )
{
	int result = -1;

	int dsf = ( data[ 3 ] & 0x80 ) >> 7;
	int stype = data[ 80*5 + 48 + 3 ] & 0x1f;
	int apt = data[ 4 ] & 0x07;

	if ( dsf == 1 && stype == 0 && apt ) 
		result = 2;

	for ( int i = 0; result == -1 && dv_profiles[ i ].dsf != -1; i ++ )
		if ( dv_profiles[ i ].dsf == dsf && dv_profiles[ i ].stype == stype )
			result = i;

	return result;
}

} } }
