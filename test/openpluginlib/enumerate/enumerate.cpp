
// enumerate - openpluginlib unit test.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

// unit test to enumerate all available plugins

#include <iostream>

#include <openpluginlib/pl/openpluginlib.hpp>

namespace opl = olib::openpluginlib;

int main( )
{
	opl::init( "" );

	opl::discovery<> rez0;
	rez0.sort( opl::highest_merit_sort( ) );

	std::cout << "enumerate: number of plugins available " << static_cast<unsigned int>( rez0.size( ) ) << ".\n";

	opl::discovery<>::const_iterator i = rez0.begin( );
	while( i != rez0.end( ) )
	{
		std::wcout << "\tname: " << i->name( ) << L" type: " << i->type( ) << L".\n";
		++i;
	}

	opl::uninit( );
}
