
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <cassert>
#include <iostream>

#include <openpluginlib/pl/openpluginlib.hpp>

namespace opl = olib::openpluginlib;

int main( int argc, char* argv[ ] )
{
	if( argc > 1 )
	{
		if( !opl::init( argv[ 1 ] ) ) return 1;

		opl::discovery<> rez0;
		assert( rez0.size( ) == 4 && L"discovery: number of plugins is different from 4." );

		std::cout << "discovery: number of plugins available " << static_cast<unsigned int>( rez0.size( ) ) << ".\n";

		typedef opl::discovery<opl::openmedialib_default_query_traits> oml_discovery;
		oml_discovery rez1;
		assert( rez1.size( ) == 3 && L"discovery: number of openmedialib plugins is different from 3." );

		std::cout << "discovery: number of openmedialib plugins available " << static_cast<unsigned int>( rez1.size( ) ) << ".\n";

		oml_discovery::const_iterator i1 = rez1.begin( );
		while( i1 != rez1.end( ) )
		{
			std::wcout << "\tname: " << i1->name( ) << L" type: " << i1->type( ) << L" mime: " << i1->mime( ) << L".\n";
			
			++i1;
		}

		opl::uninit( );
	}
}
