#ifndef AVFORMAT_UTILS_HPP_INCLUDED
#define AVFORMAT_UTILS_HPP_INCLUDED

#include <openmedialib/ml/openmedialib_plugin.hpp>

extern "C" {
#include <libavformat/avformat.h>
}

#include <string>

namespace olib { namespace openmedialib { namespace ml {

extern void prores_stream_analyze( const AVPacket *pkt, AVCodecContext *codec );
extern std::string avformat_codec_id_to_apf_codec( AVCodecID codec_id, unsigned int codec_tag );
extern AVCodecID stream_to_avformat_codec_id( const stream_type_ptr &stream );
extern olib::openmedialib::ml::image_type_ptr convert_to_oil( struct SwsContext *&, AVFrame *, PixelFormat, int, int );
extern const olib::t_string avformat_to_oil( int );
extern const PixelFormat oil_to_avformat( const olib::t_string & );
extern AVSampleFormat aml_id_to_AVSampleFormat( audio::identity );
extern audio::identity AVSampleFormat_to_aml_id( AVSampleFormat fmt );
extern std::string AVError_to_string( int errnum );
} } }

#endif

