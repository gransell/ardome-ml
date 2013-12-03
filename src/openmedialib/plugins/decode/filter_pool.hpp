#ifndef plugin_decode_filter_pool_h
#define plugin_decode_filter_pool_h

#include <openmedialib/ml/filter.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <set>

#define LOKI_CLASS_LEVEL_THREADING
#include <loki/Singleton.h>

namespace pl = olib::openpluginlib;
namespace ml = olib::openmedialib::ml;
namespace il = olib::openimagelib::il;
namespace pcos = olib::openpluginlib::pcos;
namespace cl = olib::opencorelib;

namespace olib { namespace openmedialib { namespace ml { namespace decode {

class filter_pool
{
public:
	virtual ~filter_pool( ) { }
	
	friend class shared_filter_pool;
	friend class frame_lazy;
	
protected:
	virtual ml::filter_type_ptr filter_obtain( ) = 0;
	virtual void filter_release( ml::filter_type_ptr ) = 0;
};

typedef boost::shared_ptr< filter_pool > filter_pool_ptr;

} } } }

#endif

