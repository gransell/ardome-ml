
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <cstdlib>
#include <vector>

#include <openpluginlib/pl/utf8_utils.hpp>

namespace olib { namespace openpluginlib {

wstring to_wstring( const string& str )
{
	std::vector<wchar_t> ws;
				
	ws.resize( str.size( ), 0 );

#if _MSC_VER >= 1400
	ws.push_back( 0 ); // microsoft secure code initiative or whatever...

	size_t size;
	mbstowcs_s( &size, &ws[ 0 ], ws.size( ), str.c_str( ), str.size( ) );
	
	return wstring( &ws[ 0 ] );
#else
	mbstowcs( &ws[ 0 ], str.c_str( ), str.size( ) );
	
	return wstring( ws.begin( ), ws.end( ) );
#endif
}
			
string to_string( const wstring& str )
{
	std::vector<char> mbs;
			
	mbs.resize( str.size( ) );

#if _MSC_VER >= 1400
	mbs.push_back( 0 ); // microsoft secure code initiative or whatever...

	size_t size;
	wcstombs_s( &size, &mbs[ 0 ], mbs.size( ), str.c_str( ), str.size( ) );
	
	return string( &mbs[ 0 ] );
#else
	wcstombs( &mbs[ 0 ], str.c_str( ), str.size( ) );
	
	return string( mbs.begin( ), mbs.end( ) );
#endif
}

} }
