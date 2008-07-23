
#ifndef OPENMEDIALIB_TYPES_H_
#define OPENMEDIALIB_TYPES_H_

#include <openmedialib/ml/config.hpp>
#include <boost/shared_ptr.hpp>

#include <utility>
#include <vector>

namespace olib { namespace openmedialib { namespace ml {

class ML_DECLSPEC frame_type;
class ML_DECLSPEC input_type;
class ML_DECLSPEC filter_type;
class ML_DECLSPEC store_type;

typedef ML_DECLSPEC boost::shared_ptr< frame_type > frame_type_ptr;
typedef ML_DECLSPEC boost::shared_ptr< input_type > input_type_ptr;
typedef ML_DECLSPEC boost::shared_ptr< filter_type > filter_type_ptr;
typedef ML_DECLSPEC boost::shared_ptr< store_type > store_type_ptr;

typedef ML_DECLSPEC boost::shared_ptr< std::exception > exception_ptr;
typedef ML_DECLSPEC std::pair< exception_ptr, input_type_ptr > exception_item;
typedef ML_DECLSPEC std::vector< exception_item > exception_list;

} } }

#endif
