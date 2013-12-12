#include <openmedialib/ml/store.hpp>
#include <openmedialib/ml/frame.hpp>
#include <openmedialib/ml/input.hpp>
#include <openmedialib/ml/filter.hpp>

#include <openmedialib/ml/openmedialib_plugin.hpp>

#include "mock_store.hpp"
#include "mock_filter.hpp"
#include "mock_input.hpp"

namespace opl = olib::openpluginlib;

namespace olib { namespace openmedialib { namespace ml { namespace unittest {

class ML_PLUGIN_DECLSPEC mock_plugin : public openmedialib_plugin
{
public:

	virtual input_type_ptr input( const std::wstring &resource )
	{
		static ml::input_type_ptr f = ml::input_type_ptr( new mock_input );
		return f;		
	}

	virtual store_type_ptr store( const std::wstring &resource, const frame_type_ptr &frame )
	{
		static store_type_ptr s = store_type_ptr( new mock_store );
		mock_store* m = static_cast <mock_store*> (s.get());
		m->m_create_count ++;
		return s;
	}
	
	virtual filter_type_ptr filter( const std::wstring& resource )
	{
		static filter_type_ptr f = filter_type_ptr( new mock_filter );
		return f;
	}
};

extern "C"
{
	ML_PLUGIN_DECLSPEC bool openplugin_init( void )
	{
		return true;
	}

	ML_PLUGIN_DECLSPEC bool openplugin_uninit( void )
	{
		return true;
	}
	
	ML_PLUGIN_DECLSPEC bool openplugin_create_plugin( const char*, opl::openplugin** plug )
	{
		*plug = new mock_plugin;
		return true;
	}
	
	ML_PLUGIN_DECLSPEC void openplugin_destroy_plugin( opl::openplugin* plug )
	{ 
		delete static_cast< mock_plugin * >( plug ); 
	}
}

} } } }
