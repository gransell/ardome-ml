#include "input_creator_handler.hpp"

namespace olib { namespace openmedialib { namespace ml {

typedef Loki::SingletonHolder< input_creator_handler > the_internal_input_creator_handler;

input_creator_handler &the_input_creator_handler::instance( )
{
	return the_internal_input_creator_handler::Instance( );
}

} } }
