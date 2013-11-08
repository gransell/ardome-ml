#ifndef RESCALE_OBJECT_H_
#define RESCALE_OBJECT_H_

#include <openmedialib/ml/config.hpp>
#include <openmedialib/ml/image/image_types.hpp>

namespace olib { namespace openmedialib { namespace ml { namespace image {

ML_DECLSPEC bool is_alpha( MLPixelFormat pf );

/*
* Will provide a significant speedup to sws_scale if provided with each convert/rescale
*/
class ML_DECLSPEC rescale_object
{
	public:
		rescale_object( );
		~rescale_object( );
		void *get_context( MLPixelFormat pf );
		void set_context( MLPixelFormat pf, void *context );
	private:
		void *image_;
		void *alpha_;
};

} } } }

#endif
