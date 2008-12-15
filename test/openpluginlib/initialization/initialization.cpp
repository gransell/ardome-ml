
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <openpluginlib/pl/openpluginlib.hpp>

namespace opl = olib::openpluginlib;

int main( )
{
	if( opl::init( "" ) )
		opl::uninit( );

	if( opl::init( "" ) )
			opl::uninit( );
}
