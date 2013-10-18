
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include "property_container.hpp"

// framework
#include <openpluginlib/pl/pcos/observer.hpp>
#include <openpluginlib/pl/pcos/subject.hpp>

// std
#include <map>
#include <algorithm>
#include <iterator>
#include <memory>
#include <iostream>

namespace olib { namespace openpluginlib { namespace pcos {

/// a simple forwarding observer; connect to all contained properties
class forwarding_observer : public observer
{
public:
    forwarding_observer( isubject* forwardTo )
        : forwardTo_( forwardTo )
        {}

    virtual void updated( isubject* )
    {
        forwardTo_->update();
    }

private:
    isubject* forwardTo_;
};

/// inner class  
class property_container::property_container_impl
{
public:
    property_container_impl( property_container* )
        : propertyObserver( new forwarding_observer( &subject_ ) )
    {}

	~property_container_impl()
	{
		//Make sure to remove the observers on the properties, since the observer
		//callback will be invalid after this container has been destroyed
		for( property_map::iterator I = propertyMap.begin(); I != propertyMap.end(); ++I )
		{
			I->second.detach( propertyObserver );
		}
	}

    void append( property p )
    {
		property_map::iterator iter = propertyMap.find( p.get_key() );
		if( iter != propertyMap.end() )
		{
			//If there was a previous property with the same key, we must
			//detach it from our observer before letting it go. This is 
			//because the property might live longer than this container, and
			//could attempt to update or propertyObserver after we've been
			//destroyed, which will cause a crash.
			remove( iter->second );
		}

		propertyMap.insert( std::make_pair( p.get_key(), p ) );
        p.attach( propertyObserver );
    }

    void remove( property p )
    {
	property_map::iterator I = propertyMap.find( p.get_key() );

        if ( I != propertyMap.end() )
        {
            propertyMap.erase( p.get_key() );
            p.detach( propertyObserver );
        }
    }

    template < typename T > struct CloneProperty
    {
	property_container_impl* impl_;
	
	CloneProperty( property_container_impl* impl )
	    : impl_( impl )
	    {}
	
	void operator()( const typename T::value_type& mi )
	{
	    std::auto_ptr< property > clone( mi.second.clone() );
	    impl_->append( *clone.get() );
	}
    };


    property_container_impl* clone( property_container* pc ) const
    {
	property_container_impl* result = new property_container_impl( pc );
	std::for_each( propertyMap.begin(), propertyMap.end(), CloneProperty< property_map >( result ) );

	return result;
    }

    typedef std::map< key, property > property_map;
    property_map propertyMap;
    subject subject_;

private:
    boost::shared_ptr< forwarding_observer > propertyObserver;
};
  
/// main class definition
property_container::property_container()
{
	impl_.reset( new property_container_impl( this ) );
}

property_container::~property_container()
{}

bool property_container::operator==( const property_container& rhs ) const
{
    return impl_ == rhs.impl_;
}

bool property_container::operator!=( const property_container& rhs ) const
{
    return !operator==( rhs );
}

void property_container::attach( boost::shared_ptr< observer > obs )
{
	impl_->subject_.attach( obs );
}

void property_container::detach( boost::shared_ptr< observer > obs )
{
	impl_->subject_.detach( obs );
}

void property_container::update()
{
	impl_->subject_.update();
}

void property_container::block( boost::shared_ptr< observer > obs )
{
	impl_->subject_.block( obs );
}

void property_container::unblock( boost::shared_ptr< observer > obs )
{
	impl_->subject_.unblock( obs );
}

property property_container::get_property_with_string( const char* k ) const
{
    return get_property_with_key( key::from_string( k ) );
}
    
property property_container::get_property_with_key( const key& k ) const
{
    if ( !impl_->propertyMap.count( k ) )
    {
        return property::NULL_PROPERTY;
    }

    return impl_->propertyMap.find( k )->second;
}

template < typename T, typename U > T returnFirst( const std::pair< T, U >& p )
{
    return p.first;
}

key_vector property_container::get_keys() const
{
    key_vector result;
    std::back_insert_iterator< key_vector > bii( result );
    std::transform( impl_->propertyMap.begin(), impl_->propertyMap.end(), bii, returnFirst< key, property > );

    return result;
}

void property_container::append( const property& p )
{
    impl_->append( p );
}

void property_container::remove( const property& p )
{
    impl_->remove( p );
}

void property_container::accept( visitor& v )
{
    v.visit_property_container( *this );
}

property_container* property_container::clone() const
{
    property_container* result = new property_container();
    result->impl_.reset( impl_->clone( result ) );

    return result;
}

std::ostream& operator<<( std::ostream& os, const property_container& pc )
{
    os << "property_container: " << &pc << "\n";
    
    key_vector keys = pc.get_keys();
    key_vector::const_iterator I = keys.begin();
    key_vector::const_iterator E = keys.end();
    for( ; I!=E; ++I )
    {
	os << pc.get_property_with_key( *I ) << "\n";
    }

    return os;
}

} } } 

