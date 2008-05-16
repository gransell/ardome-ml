
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <cassert>
#include <iostream>
#include <string>

#include <openpluginlib/pl/pcos/key.hpp>

namespace opl = olib::openpluginlib::pcos;

int main( )
{
    // equality
    {
        opl::key key1( opl::key::from_string( "foobar" ) );
        opl::key key2( opl::key::from_string( "foobar" ) );
        assert( key1 == key2 );
        assert( !(key1 < key2 ) );
        assert( !(key1 != key2 ) );
    }

    // inequality
    {
        opl::key key1( opl::key::from_string( "foobar1" ) );
        opl::key key2( opl::key::from_string( "foobar2" ) );
        assert( !(key1 == key2) );
        assert( key1 != key2 );
    }

    // name
    {
        opl::key key( opl::key::from_string( "foobar" ) );
        assert( std::string( "foobar" ) == std::string( key.as_string() ) );
    }
    
    return 0;
}
