
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef OPENPLUGIN_INC_
#define OPENPLUGIN_INC_

#include <vector>

#ifdef WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif // WIN32

#include <boost/shared_ptr.hpp>
#ifndef BOOST_REGEX_DYN_LINK 
    #define BOOST_REGEX_DYN_LINK 
#endif
#include <boost/regex.hpp>
#ifndef BOOST_THREAD_DYN_DLL
    #define BOOST_THREAD_DYN_DLL
#endif

#include <openpluginlib/pl/config.hpp>

namespace olib { namespace openpluginlib {

class openplugin;

extern "C"
{
	typedef bool ( *openplugin_init )( );
	typedef bool ( *openplugin_uninit )( );
	typedef bool ( *openplugin_create_plugin )( const char*, openplugin** );
	typedef void ( *openplugin_destroy_plugin )( openplugin* );
}

namespace detail
{
	struct OPENPLUGINLIB_DECLSPEC plugin_resolver
	{
		explicit plugin_resolver( );
		~plugin_resolver( );

		openplugin_init				init;
		openplugin_uninit			uninit;
		openplugin_create_plugin	create_plugin;
		openplugin_destroy_plugin	destroy_plugin;
		
#	ifdef WIN32
		HMODULE	dl_handle;
#	else
		void*	dl_handle;
#	endif
		bool dlopened;		
	};

	bool load_shared_library( plugin_resolver& resolver, const std::vector<std::wstring>& shared_name );

	void unload_shared_library( );

	struct plugin_item
	{
		explicit plugin_item( )
			: merit( 0 )
			, context( 0 )
		{ }

		std::vector<std::wstring> filenames;
		std::vector<boost::wregex> extension;
		std::wstring name, type, mime, category, libname, in_filter, out_filter;
		std::wstring opl_path; //The path to the opl file that references this plugin
		int merit;

		// 3rd party plugin API support.
		void* context;

		plugin_resolver resolver;
	};
}

class OPENPLUGINLIB_DECLSPEC openplugin
{
public:
	explicit openplugin( ) { }
	virtual ~openplugin( ) { }
};

typedef boost::shared_ptr<openplugin> opl_ptr;

} }

#endif
