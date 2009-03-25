
// quicktime - A Quicktime plugin to ml.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

// SYSTEM INCLUDES
#ifdef WIN32
#include <windows.h>
#endif // WIN32

// SYSTEM INCLUDES
#include <boost/thread/recursive_mutex.hpp>							// blocks plugin functions

// OL INCLUDES
#include <openmedialib/ml/openmedialib_plugin.hpp>					// images, audio etc
#include <openpluginlib/pl/string.hpp>								// converts file location

// LOCAL INCLUDES
#include <openmedialib/plugins/quicktime/quicktime_input.h>			// quicktime input
#include <openmedialib/plugins/quicktime/quicktime_store.h>			// quicktime store

namespace opl = olib::openpluginlib;
namespace plugin = olib::openmedialib::ml;

namespace olib { namespace openmedialib { namespace ml { 
using namespace std;

//
// Plugin object
//

class ML_PLUGIN_DECLSPEC quicktime_plugin : public openmedialib_plugin
{
public:

	virtual input_type_ptr input( const opl::wstring &resource )
	{
 		typedef boost::shared_ptr< quicktime_input > result_type_ptr;
		result_type_ptr result = result_type_ptr( new quicktime_input( resource ) );
		if ( !result->is_valid( ) )
			result = result_type_ptr( );
		return result;
	}

	
	virtual store_type_ptr store( const opl::wstring &resource, const frame_type_ptr &frame )
	{
		typedef boost::shared_ptr< quicktime_store > result_type_ptr;
		return result_type_ptr( new quicktime_store( resource, frame ) );
	}
};

} } }

//
// Library initialisation mechanism
//

namespace
{
	void reflib( int init )
	{
		static long refs = 0;

		assert( refs >= 0 && L"quicktime_plugin::refinit: refs is negative." );
		
		if( init > 0 && ++refs == 1)
		{
		}
		else if( init < 0 && --refs == 0 )
		{
			
		}
	}
	
	boost::recursive_mutex mutex;
}

//
// Access methods for openpluginlib
//

extern "C"
{
	ML_PLUGIN_DECLSPEC bool openplugin_init( void )
	{
		boost::recursive_mutex::scoped_lock lock( mutex );
		reflib( 1 );
		return true;
	}

	ML_PLUGIN_DECLSPEC bool openplugin_uninit( void )
	{
		boost::recursive_mutex::scoped_lock lock( mutex );
		reflib( -1 );
		return true;
	}
	
	ML_PLUGIN_DECLSPEC bool openplugin_create_plugin( const char*, opl::openplugin** plug )
	{
		*plug = new plugin::quicktime_plugin;
		return true;
	}
	
	ML_PLUGIN_DECLSPEC void openplugin_destroy_plugin( opl::openplugin* plug )
	{ 
		delete static_cast< plugin::quicktime_plugin * >( plug ); 
	}
}

