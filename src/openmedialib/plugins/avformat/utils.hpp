#ifndef AVFORMAT_UTILS_HPP_INCLUDED
#define AVFORMAT_UTILS_HPP_INCLUDED

extern "C" {
#include <libavformat/avformat.h>
}

#include <string>

namespace olib { namespace openmedialib { namespace ml {

std::string avformat_codec_id_to_apf_codec( CodecID codec_id );
CodecID stream_to_avformat_codec_id( const stream_type_ptr &stream );

} } }

#endif

