
#ifndef OPENMEDIALIB_TYPES_H_
#define OPENMEDIALIB_TYPES_H_

#include <openmedialib/ml/config.hpp>
#include <boost/shared_ptr.hpp>

#include <utility>
#include <vector>

namespace olib { namespace openmedialib { namespace ml {

class ML_DECLSPEC frame_type;
class ML_DECLSPEC stream_type;
class ML_DECLSPEC input_type;
class ML_DECLSPEC filter_type;
class ML_DECLSPEC store_type;

// Being replaced by stream_type
class ML_DECLSPEC packet_type;
class ML_DECLSPEC packet_decoder;
class ML_DECLSPEC packet_encoder;

typedef ML_DECLSPEC boost::shared_ptr< frame_type > frame_type_ptr;
typedef ML_DECLSPEC boost::shared_ptr< stream_type > stream_type_ptr;
typedef ML_DECLSPEC boost::shared_ptr< input_type > input_type_ptr;
typedef ML_DECLSPEC boost::shared_ptr< filter_type > filter_type_ptr;
typedef ML_DECLSPEC boost::shared_ptr< store_type > store_type_ptr;

// Being replaced by stream_type_ptr and associated filters
typedef ML_DECLSPEC boost::shared_ptr< packet_type > packet_type_ptr;
typedef ML_DECLSPEC boost::shared_ptr< packet_decoder > packet_decoder_ptr;
typedef ML_DECLSPEC boost::shared_ptr< packet_encoder > packet_encoder_ptr;

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

} } }

#endif
