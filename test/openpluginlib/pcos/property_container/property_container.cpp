
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <cassert>
#include <iostream>

#include <openpluginlib/pl/pcos/property_container.hpp>
#include <openpluginlib/pl/pcos/observer.hpp>
#include <openpluginlib/pl/string.hpp>

namespace opl = olib::openpluginlib;
namespace pcos = olib::openpluginlib::pcos;

class counting_observer : public pcos::observer
{
public:
    counting_observer()
        : count( 0 )
        {}

    virtual void updated( pcos::isubject* )
    {
        ++count;
    }

    unsigned int count;
};

int main( )
{
    // NULL containment test
    {
        pcos::property_container pc;
        assert( pc.get_property_with_key( pcos::key::from_string( "key1" ) ) == pcos::property::NULL_PROPERTY );
        assert( pc.get_property_with_string( "key1" ) == pcos::property::NULL_PROPERTY );
    }

    // basic containment test
    {
        pcos::property_container pc;
        pcos::property p1( pcos::key::from_string( "key1" ) );
        pcos::property p2( pcos::key::from_string( "key2" ) );
        assert( p1 != p2 );

        pc.append( p1 );
        pc.append( p2 );

        assert( pc.get_keys().size() == 2 );
        assert( pc.get_property_with_string( "key1" ) == p1 );
        assert( pc.get_property_with_string( "key2" ) == p2 );

        pc.remove( p2 );

        assert( pc.get_keys().size() == 1 );
        assert( pc.get_property_with_string( "key1" ) == p1 );
        assert( pc.get_property_with_string( "key2" ) == pcos::property::NULL_PROPERTY );
    }

    // notification
    {
        boost::shared_ptr< counting_observer > obs( new counting_observer );
        
        pcos::property_container pc;
        pcos::property p1( pcos::key::from_string( "key1" ) );
        pcos::property p2( pcos::key::from_string( "key2" ) );

        pc.append( p1 );
        pc.append( p2 );

        pc.attach( obs );
        
        p1 = 3.141;
        assert( obs->count == 1 );

        p2 = opl::wstring( L"foobar" );
        assert( obs->count == 2 );
    }

    // cloning
    {
        pcos::property p1( pcos::key::from_string( "key1" ) );
        pcos::property p2( pcos::key::from_string( "key2" ) );

        pcos::property_container pc;
        pc.append( p1 );
        pc.append( p2 );
        
        p1 = 3.141;
        p2 = opl::wstring( L"foobar" );

	pcos::property_container* pc_clone = pc.clone();

	// check the containers are not using the same impl
	assert( pc != *pc_clone );

	// ensure the clone has the right number of properties
	assert( pc_clone->get_keys().size() == 2 );

	// check the properties exist but are not impl equal to the originals
	assert( pc_clone->get_property_with_string( "key1" ) != pcos::property::NULL_PROPERTY );
	assert( pc_clone->get_property_with_string( "key1" ) != p1 );
	assert( pc_clone->get_property_with_string( "key2" ) != pcos::property::NULL_PROPERTY );
	assert( pc_clone->get_property_with_string( "key2" ) != p2 );

	// check the values are equal
	assert( pc_clone->get_property_with_string( "key1" ).value< double >() == p1.value< double >() );
	assert( pc_clone->get_property_with_string( "key2" ).value< opl::wstring >() == p2.value< opl::wstring >() );

	// check the notification is correctly detached now
        boost::shared_ptr< counting_observer > clone_obs( new counting_observer );
        boost::shared_ptr< counting_observer > obs( new counting_observer );
        pc_clone->attach( clone_obs );
        pc.attach( obs );
	
	pc_clone->get_property_with_string( "key1" ) = 2.171;

        assert( clone_obs->count == 1 );
        assert( obs->count == 0 );
	assert( pc_clone->get_property_with_string( "key1" ).value< double >() != p1.value< double >() );
    }

    return 0;
}
