
#ifndef OPENMEDIALIB_TYPES_H_
#define OPENMEDIALIB_TYPES_H_

#include <opencorelib/cl/lru_cache.hpp>
#include <openmedialib/ml/config.hpp>
#include <openmedialib/ml/audio_types.hpp>
#include <openmedialib/ml/image/image_types.hpp>
#include <openmedialib/ml/image/rescale_object.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>

#include <utility>
#include <vector>

namespace olib { namespace openmedialib { namespace ml {

// Forward declaration of core AML types 
class ML_DECLSPEC frame_type;
class ML_DECLSPEC stream_type;
class ML_DECLSPEC input_type;
class ML_DECLSPEC filter_type;
class ML_DECLSPEC store_type;

// Shared pointer wrappers of core AML types 
typedef ML_DECLSPEC boost::shared_ptr< frame_type > frame_type_ptr;
typedef ML_DECLSPEC boost::shared_ptr< stream_type > stream_type_ptr;
typedef ML_DECLSPEC boost::shared_ptr< input_type > input_type_ptr;
typedef ML_DECLSPEC boost::shared_ptr< filter_type > filter_type_ptr;
typedef ML_DECLSPEC boost::shared_ptr< store_type > store_type_ptr;

// Declaration of the neutral audio_type_ptr
typedef ML_DECLSPEC boost::shared_ptr < audio::base > audio_type_ptr;

// Declaration of the neutral image_type_ptr
typedef ML_DECLSPEC boost::shared_ptr < image::image > image_type_ptr;

// Rescaler object for converting/rescaling speedup
typedef ML_DECLSPEC boost::shared_ptr < image::rescale_object > rescale_object_ptr;

// Exception handling types
typedef ML_DECLSPEC boost::shared_ptr< std::exception > exception_ptr;
typedef ML_DECLSPEC std::pair< exception_ptr, input_type_ptr > exception_item;
typedef ML_DECLSPEC std::vector< exception_item > exception_list;

/// AWI definitions
class ML_DECLSPEC awi_index;
class ML_DECLSPEC awi_parser_v2;
class ML_DECLSPEC awi_generator_v2;
class ML_DECLSPEC awi_parser_v3;
class ML_DECLSPEC awi_generator_v3;

typedef ML_DECLSPEC boost::shared_ptr< awi_index > awi_index_ptr;
typedef ML_DECLSPEC boost::shared_ptr< awi_parser_v2 > awi_parser_v2_ptr;
typedef ML_DECLSPEC boost::shared_ptr< awi_parser_v3 > awi_parser_v3_ptr;
typedef ML_DECLSPEC boost::shared_ptr< awi_generator_v2 > awi_generator_v2_ptr;
typedef ML_DECLSPEC boost::shared_ptr< awi_generator_v3 > awi_generator_v3_ptr;

class ML_DECLSPEC indexer_item;
typedef ML_DECLSPEC boost::shared_ptr< indexer_item > indexer_item_ptr;

/// Used to return frame rate and sample aspect ratio
struct fraction
{
	fraction( int n, int d )
	{
		num = n;
		den = d;
	}

	int num;
	int den;
};

/// fraction typedef
typedef struct fraction fraction;

/// Used to return video dimensions
struct dimensions
{
	dimensions( int w, int h )
	{
		width = w;
		height = h;
	}

	int width;
	int height;
};

/// dimensions typedef
typedef struct dimensions dimensions;

/// Frame Cache
typedef olib::opencorelib::lru< int, frame_type_ptr > lru_frame_type;
typedef olib::opencorelib::lru_cache< lru_frame_type > lru_frame_cache;
typedef boost::shared_ptr< lru_frame_type > lru_frame_ptr;

/// Audio Cache
typedef olib::opencorelib::lru< int, audio_type_ptr > lru_audio_type;
typedef olib::opencorelib::lru_cache< lru_audio_type > lru_audio_cache;
typedef boost::shared_ptr< lru_audio_type > lru_audio_ptr;

/// Stream Cache
typedef olib::opencorelib::lru< boost::int64_t, stream_type_ptr > lru_stream_type;
typedef olib::opencorelib::lru_cache< lru_stream_type > lru_stream_cache;
typedef boost::shared_ptr< lru_stream_type > lru_stream_ptr;

} } }

#endif
