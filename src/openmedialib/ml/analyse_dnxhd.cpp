// ml - A media library representation.

// Copyright (C) 2011 Vizrt
// Released under the LGPL.

#include "analyse_dnxhd.hpp"
#include <opencorelib/cl/enforce_defines.hpp>

namespace pl = olib::openpluginlib;

namespace olib { namespace openmedialib { namespace ml {

typedef struct dnxhd_format_t dnxhd_format_t;

struct dnxhd_format_t 
{
   int cid;
   int framesize;
   int width;
   int height;
   int bits;
   bool interlaced;
};

#define NUM_VC3_FORMATS 11

const dnxhd_format_t dnxhd_formats[ NUM_VC3_FORMATS ] = 
{
	// Compression ID, Frame size,
	// Active lines, Sample bit depth,
	// Interlaced flag
	{ 1235, 917504, 1920, 1080,  10, false },
	{ 1237, 606208, 1920, 1080,   8, false },
	{ 1238, 917504, 1920, 1080,   8, false },
	{ 1241, 917504, 1920, 1080,  10, true  },
	{ 1242, 606208, 1920, 1080,   8, true  },
	{ 1243, 917504, 1920, 1080,   8, true  },
	{ 1244, 606208, 1440, 1080,   8, true  },
	{ 1250, 458752, 1280,  720,  10, false },
	{ 1251, 458752, 1280,  720,   8, false },
	{ 1252, 303104, 1280,  720,   8, false },
	{ 1253, 188416, 1920, 1080,   8, false }
};

// FPS 24, 25, 29.97, 50, 59.94
// cross referenced to CID

int cidfps[ NUM_VC3_FORMATS ][ 5 ] = 
{
	{ 175, 185, 0, 0, 0 },
	{ 115, 120, 0, 0, 0 },
	{ 175, 185, 0, 0, 0 },
	{ 0,   185, 220, 0, 0 },
	{ 0,   120, 145, 0, 0 },
	{ 0,   185, 220, 0, 0 },
	{ 0,   185, 220, 0, 0 },
	{ 90,  90, 0, 180, 220 },
	{ 90,  90, 0, 180, 220 },
	{ 60,  60, 0, 120, 145 },
	{ 35,  0, 0, 0, 0 }
};

int cid_index( int cid ) 
{
   if ( cid > 1249 )
      return cid - 1245;
   if ( cid > 1240 )
      return cid - 1238;
   if ( cid > 1236 )
      return cid - 1236;
   return 0; //cid - 1235
}

static const ml::fraction dnxhd_fps[ 5 ] =
{
	ml::fraction( 24000, 1001 ),
	ml::fraction( 25, 1 ),
	ml::fraction( 30000, 1001 ),
	ml::fraction( 50, 1 ),
	ml::fraction( 60000, 1001 )
};

int vidformat2index( const int fps_num, const int fps_den ) 
{
	int normrate = int( float( fps_num ) / fps_den + 0.5 );

	if ( normrate == 24 )
		return 0;
	if ( normrate == 25 )
		return 1;
	if ( normrate == 30 )
		return 2;
	if ( normrate == 50 )
		return 3;
	if ( normrate == 60 )
		return 4;

	ARENFORCE_MSG( false, "Unsupported framerate" )( fps_num )( fps_den );
    return 0;
}

analyse_dnxhd::analyse_dnxhd( )
: id_( -1 )
, field_( ml::image::progressive )
{
}

bool analyse_dnxhd::valid( const std::string &codec )
{
	return true;
}

bool analyse_dnxhd::analysis( boost::uint8_t *data, const size_t size )
{
	id_ = cid( data, size );
	if ( id_ != -1 )
	{
		id_ = cid_index( id_ );
		if ( data[ 5 ] & 2 )
			field_ = ( data[ 5 ] & 1 ) == 0 ? ml::image::top_field_first : ml::image::bottom_field_first;
		else
			field_ = ml::image::progressive;
	}
	return id_ != -1;
}

bool analyse_dnxhd::collect( olib::openpluginlib::pcos::property_container &properties )
{
	bool result = id_ != -1;

	if ( result )
	{
		const dnxhd_format_t *dnxhd = &dnxhd_formats[ id_ ];
		properties.append( pl::pcos::property( key_packet_size_ ) = int( dnxhd->framesize ) );
		properties.append( pl::pcos::property( key_width_ ) = dnxhd->width );
		properties.append( pl::pcos::property( key_height_ ) = dnxhd->height );
		properties.append( pl::pcos::property( key_pf_ ) = olib::t_string( "yuv422" ) );
		properties.append( pl::pcos::property( key_field_order_ ) = int( dnxhd->interlaced ? ml::image::top_field_first : ml::image::progressive ) );
		properties.append( pl::pcos::property( key_sar_num_ ) = 1 );
		properties.append( pl::pcos::property( key_sar_den_ ) = 1 );
		properties.append( pl::pcos::property( key_picture_coding_type_ ) = 0 );
		properties.append( pl::pcos::property( key_field_order_ ) = int( field_ ) );
	}

	return result;
}

int analyse_dnxhd::cid( boost::uint8_t *d, const size_t size )
{
	return size > 0x2b && d[ 0 ] == 0x00 && d[ 1 ] == 0x00 && d[ 2 ] == 0x02 && d[ 3 ] == 0x80 && d[ 4 ] == 0x01 ? get_bits( d, 0x140, 32 ) : -1;
}

int analyse_dnxhd::framesize( int cid )
{
   return dnxhd_formats[ cid_index( cid ) ].framesize;
}

int analyse_dnxhd::sample_bit_depth( int cid )
{  
   return dnxhd_formats[ cid_index( cid ) ].bits;
}  

bool analyse_dnxhd::interlace( int cid )
{
   return dnxhd_formats[ cid_index( cid ) ].interlaced;
}

int analyse_dnxhd::height( int cid )
{
   return dnxhd_formats[ cid_index( cid ) ].height;
}

int analyse_dnxhd::width( int cid )
{
   return dnxhd_formats[ cid_index( cid ) ].width;
}

int analyse_dnxhd::bitrate(int cid, const int fps_num, const int fps_den) 
{
      int vfindex = vidformat2index( fps_num, fps_den );
      int cidindex = cid_index( cid );
      long bitrate = cidfps[ cidindex ][ vfindex ] * 1000000;

      ARENFORCE_MSG( bitrate , "Unsupported framerate for Compression ID" )( cid )( fps_num )( fps_den );

      return bitrate;
}

} } }

