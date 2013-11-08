#include "rescale_object.hpp"

extern "C" {
#include <libswscale/swscale.h>
}

namespace olib { namespace openmedialib { namespace ml { namespace image {

ML_DECLSPEC bool is_alpha( MLPixelFormat pf )
{
	return pf == ml::image::ML_PIX_FMT_L8 || pf == ml::image::ML_PIX_FMT_L16LE;
}

rescale_object::rescale_object( ) 
: image_ ( 0 )
, alpha_ ( 0 )
{
}

rescale_object::~rescale_object () 
{	
	sws_freeContext( (struct SwsContext *)image_ );
	sws_freeContext( (struct SwsContext *)alpha_ );
}

void *rescale_object::get_context( MLPixelFormat pf ) 
{ 
	return is_alpha( pf ) ? alpha_ : image_; 
}

void rescale_object::set_context( MLPixelFormat pf, void *context ) 
{ 
	if ( is_alpha( pf ) ) 
		alpha_ = context; 
	else 
		image_ = context; 
}

} } } }
