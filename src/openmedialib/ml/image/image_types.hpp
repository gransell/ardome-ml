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
    ML_PIX_FMT_NONE,
    ML_PIX_FMT_YUV420P,
    ML_PIX_FMT_YUV420P10,
    ML_PIX_FMT_YUVA420P,
    ML_PIX_FMT_UYV422,
    ML_PIX_FMT_YUV422P,
    ML_PIX_FMT_YUV422,
    ML_PIX_FMT_YUV422P10,
    ML_PIX_FMT_YUV422P10LE,
	ML_PIX_FMT_YUV444,
	ML_PIX_FMT_YUV444P,
	ML_PIX_FMT_YUV411,
	ML_PIX_FMT_YUV411P,
    ML_PIX_FMT_L8,
    ML_PIX_FMT_L8A8,
    ML_PIX_FMT_L16,
    ML_PIX_FMT_R8G8B8,
	ML_PIX_FMT_R8G8B8A8,
    ML_PIX_FMT_B8G8R8A8,
    ML_PIX_FMT_A8R8G8B8,
    ML_PIX_FMT_A8B8G8R8,
    ML_PIX_FMT_R10G10B10,
};


namespace {
std::map<t_string, MLPixelFormat> MLPixelFormatMap = boost::assign::map_list_of
(_CT("yuv420p"),    ML_PIX_FMT_YUV420P)
(_CT("yuv420p10"),  ML_PIX_FMT_YUV420P10)
(_CT("yuva420p"),   ML_PIX_FMT_YUVA420P)
(_CT("yuv422p"),    ML_PIX_FMT_YUV422P)
(_CT("yuv422"),     ML_PIX_FMT_YUV422)
(_CT("uyv422"),     ML_PIX_FMT_UYV422)
(_CT("yuv444"),     ML_PIX_FMT_YUV444)
(_CT("yuv444p"),    ML_PIX_FMT_YUV444P)
(_CT("yuv411"),     ML_PIX_FMT_YUV411)
(_CT("yuv411p"),    ML_PIX_FMT_YUV411P)
(_CT("yuv422p10"),  ML_PIX_FMT_YUV422P10)
(_CT("yuv422p10le"),ML_PIX_FMT_YUV422P10LE)
(_CT("l8"),         ML_PIX_FMT_L8)
(_CT("l8a8"),       ML_PIX_FMT_L8A8)
(_CT("l16"),        ML_PIX_FMT_L16)
(_CT("r8g8b8"),     ML_PIX_FMT_R8G8B8)
(_CT("r8g8b8a8"),   ML_PIX_FMT_R8G8B8A8)
(_CT("b8g8r8a8"),   ML_PIX_FMT_B8G8R8A8)
(_CT("a8r8g8b8"),   ML_PIX_FMT_A8R8G8B8)
(_CT("a8b8g8r8"),   ML_PIX_FMT_A8B8G8R8)
(_CT("r10g10b10"),  ML_PIX_FMT_R10G10B10);
}

// Forward declaration to the base image interface which all formats share
class ML_DECLSPEC image;

// Forward declaration to the template which implements the various types
template< typename T > class ML_DECLSPEC ffmpeg_image;

typedef ML_DECLSPEC ffmpeg_image<boost::uint8_t>        image_type_8;
typedef ML_DECLSPEC ffmpeg_image<boost::uint16_t>       image_type_16;
typedef ML_DECLSPEC ffmpeg_image<boost::uint32_t>       image_type_float;
typedef ML_DECLSPEC boost::shared_ptr<image_type_8>     image_type_8_ptr;
typedef ML_DECLSPEC boost::shared_ptr<image_type_16>    image_type_16_ptr;
typedef ML_DECLSPEC boost::shared_ptr<image_type_float> image_type_float_ptr;

} } } }

#endif

