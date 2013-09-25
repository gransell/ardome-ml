#ifndef AVFORMAT_UTILS_HPP_INCLUDED
#define AVFORMAT_UTILS_HPP_INCLUDED

#include <openmedialib/ml/openmedialib_plugin.hpp>

extern "C" {
#include <libavformat/avformat.h>
}

#include <string>

namespace olib { namespace openmedialib { namespace ml {

extern std::string avformat_codec_id_to_apf_codec( AVCodecID codec_id );
extern AVCodecID stream_to_avformat_codec_id( const stream_type_ptr &stream );
extern olib::openimagelib::il::image_type_ptr convert_to_oil( struct SwsContext *&, AVFrame *, PixelFormat, int, int );
extern const std::wstring avformat_to_oil( int );
extern const PixelFormat oil_to_avformat( const std::wstring & );
extern AVSampleFormat aml_id_to_AVSampleFormat( audio::identity );
extern audio::identity AVSampleFormat_to_aml_id( AVSampleFormat fmt );
extern std::string AVError_to_string( int errnum );
} } }

#endif

