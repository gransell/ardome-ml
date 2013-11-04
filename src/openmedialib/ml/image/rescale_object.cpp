#include "rescale_object.hpp"

extern "C" {
#include <libswscale/swscale.h>
}

namespace olib { namespace openmedialib { namespace ml { namespace image {
	
	rescale_object::rescale_object( ) 
		: scaler_ ( 0 )
	{
	}

	rescale_object::~rescale_object () 
	{	
		if ( scaler_ )
			sws_freeContext( (struct SwsContext *)scaler_ );
	}

} } } }
