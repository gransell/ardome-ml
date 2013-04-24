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



enum MLPixelFormat {
    ML_PIX_FMT_YUV420P,
    ML_PIX_FMT_YUVA420P,
    ML_PIX_FMT_UYV422,
    ML_PIX_FMT_YUV422P,
    ML_PIX_FMT_YUV422,
    ML_PIX_FMT_YUV422P10LE,
	ML_PIX_FMT_YUV444,
	ML_PIX_FMT_YUV444P,
	ML_PIX_FMT_YUV411,
	ML_PIX_FMT_YUV411P,
    ML_PIX_FMT_R8G8B8,
    ML_PIX_FMT_R10G10B10,
};


namespace {
std::map<t_string, MLPixelFormat> MLPixelFormatMap = boost::assign::map_list_of
("yuv420p", 	ML_PIX_FMT_YUV420P)
("yuva420p", 	ML_PIX_FMT_YUVA420P)
("yuv422p", 	ML_PIX_FMT_YUV422P)
("yuv422",		ML_PIX_FMT_YUV422)
("uyv422", 		ML_PIX_FMT_UYV422)
("uyv444",	 	ML_PIX_FMT_YUV444)
("uyv444p", 	ML_PIX_FMT_YUV444P)
("yuv411", 		ML_PIX_FMT_YUV411)
("yuv411p", 	ML_PIX_FMT_YUV411P)
("yuv422p10le", ML_PIX_FMT_YUV422P10LE)
("r8g8b8", 		ML_PIX_FMT_R8G8B8)
("r10g10b10", 	ML_PIX_FMT_R10G10B10);
}

// Forward declaration to the base image interface which all formats share
class ML_DECLSPEC image;

// Forward declaration to the template which implements the various types
template< typename T > class ML_DECLSPEC ffmpeg_image;

typedef ML_DECLSPEC ffmpeg_image<uint8_t>               image_type_8;
typedef ML_DECLSPEC ffmpeg_image<uint16_t>              image_type_16;
typedef ML_DECLSPEC ffmpeg_image<float>                 image_type_float;
typedef ML_DECLSPEC boost::shared_ptr<image_type_8>     image_type_8_ptr;
typedef ML_DECLSPEC boost::shared_ptr<image_type_16>    image_type_16_ptr;
typedef ML_DECLSPEC boost::shared_ptr<image_type_float> image_type_float_ptr;

} } } }

#endif

