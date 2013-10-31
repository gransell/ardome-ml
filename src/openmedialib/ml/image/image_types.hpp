// ml::image - basic types for image support

// Copyright (C) 2013 Vizrt
// Released under the LGPL.

#ifndef AML_IMAGE_TYPES_H_
#define AML_IMAGE_TYPES_H_

#include <openmedialib/ml/config.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/assign/list_of.hpp>
#include <opencorelib/cl/core.hpp>
#include <opencorelib/cl/minimal_string_defines.hpp>
#include <map>

namespace olib { namespace openmedialib { namespace ml { namespace image {

/*
 * Adding new pixel format:
 * Total three places needs to be modified, two in this file and one in ffmpeg_utility.cpp
 * 
 * In this file you add it to the enum MLPixelFormat and the map MLPixelFormatMap.
 *
 * Make sure you also add the mapping to ffmpegs pixel format
 * in function ML_to_AV (ffmpeg_utility.cpp)
 *
 * */

enum MLPixelFormat {
	ML_PIX_FMT_NONE = -1,
	ML_PIX_FMT_YUV420P,
	ML_PIX_FMT_YUV420P10LE,
	ML_PIX_FMT_YUV420P16LE,
	ML_PIX_FMT_YUVA420P,
	ML_PIX_FMT_UYV422,
	ML_PIX_FMT_YUV422P,
	ML_PIX_FMT_YUV422,
	ML_PIX_FMT_YUV422P10LE,
	ML_PIX_FMT_YUV422P16LE,
	ML_PIX_FMT_YUV444P,
	ML_PIX_FMT_YUVA444P,
	ML_PIX_FMT_YUV444P16LE,
	ML_PIX_FMT_YUVA444P16LE,
	ML_PIX_FMT_YUV411P,
	ML_PIX_FMT_L8,
	ML_PIX_FMT_L16LE,
	ML_PIX_FMT_R8G8B8,
	ML_PIX_FMT_B8G8R8,
	ML_PIX_FMT_R8G8B8A8,
	ML_PIX_FMT_B8G8R8A8,
	ML_PIX_FMT_A8R8G8B8,
	ML_PIX_FMT_A8B8G8R8,
	ML_PIX_FMT_R16G16B16LE,

	ML_PIX_FMT_NB, // number of pixel formats
};


namespace {
typedef std::map<t_string, MLPixelFormat> MLPixelFormatMap_type; 
MLPixelFormatMap_type MLPixelFormatMap = boost::assign::map_list_of
(_CT("yuv420p"),	ML_PIX_FMT_YUV420P)
(_CT("yuv420p10"),	ML_PIX_FMT_YUV420P10LE)
(_CT("yuv420p16"),	ML_PIX_FMT_YUV420P16LE)
(_CT("yuva420p"),	ML_PIX_FMT_YUVA420P)
(_CT("yuv422p"),	ML_PIX_FMT_YUV422P)
(_CT("yuv422"),		ML_PIX_FMT_YUV422)
(_CT("uyv422"),		ML_PIX_FMT_UYV422)
(_CT("yuv444p"),	ML_PIX_FMT_YUV444P)
(_CT("yuva444p"),	ML_PIX_FMT_YUVA444P)
(_CT("yuv444p16"),	ML_PIX_FMT_YUV444P16LE)
(_CT("yuva444p16"), ML_PIX_FMT_YUVA444P16LE)
(_CT("yuv411p"),	ML_PIX_FMT_YUV411P)
(_CT("yuv422p10"),	ML_PIX_FMT_YUV422P10LE)
(_CT("yuv422p16"),	ML_PIX_FMT_YUV422P16LE)
(_CT("l8"),			ML_PIX_FMT_L8)
(_CT("l16"),		ML_PIX_FMT_L16LE)
(_CT("r8g8b8"),		ML_PIX_FMT_R8G8B8)
(_CT("b8g8r8"),		ML_PIX_FMT_B8G8R8)
(_CT("r8g8b8a8"),	ML_PIX_FMT_R8G8B8A8)
(_CT("b8g8r8a8"),	ML_PIX_FMT_B8G8R8A8)
(_CT("a8r8g8b8"),	ML_PIX_FMT_A8R8G8B8)
(_CT("a8b8g8r8"),	ML_PIX_FMT_A8B8G8R8)
(_CT("r16g16b16"),	ML_PIX_FMT_R16G16B16LE);
}

// Forward declaration to the base image interface which all formats share
class ML_DECLSPEC image;

// Forward declaration to the template which implements the various types
template< typename T > class ML_DECLSPEC ffmpeg_image;

typedef ML_DECLSPEC ffmpeg_image<boost::uint8_t>		image_type_8;
typedef ML_DECLSPEC ffmpeg_image<boost::uint16_t>		image_type_16;
typedef ML_DECLSPEC ffmpeg_image<boost::uint32_t>		image_type_float;
typedef ML_DECLSPEC boost::shared_ptr<image_type_8>		image_type_8_ptr;
typedef ML_DECLSPEC boost::shared_ptr<image_type_16>	image_type_16_ptr;
typedef ML_DECLSPEC boost::shared_ptr<image_type_float>	image_type_float_ptr;

} } } }

#endif

