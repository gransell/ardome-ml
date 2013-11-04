#ifndef RESCALE_OBJECT_H_
#define RESCALE_OBJECT_H_

#include <openmedialib/ml/config.hpp>

namespace olib { namespace openmedialib { namespace ml { namespace image {

/*
* Will provide a significant speedup to sws_scale if provided with each convert/rescale
*/
class ML_DECLSPEC rescale_object
{
	public:
		rescale_object( );
		~rescale_object( );
		void *getContext( ) { return scaler_; }
	private:
		void *scaler_;
};

} } } }

#endif
