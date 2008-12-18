
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.


#ifndef PCOS_IPROPERTY_CONTAINER_H
#define PCOS_IPROPERTY_CONTAINER_H

#include <openpluginlib/pl/config.hpp>

// framework
#include <openpluginlib/pl/pcos/key.hpp>

// boost
#include <boost/shared_ptr.hpp>

namespace olib { namespace openpluginlib { namespace pcos {

class visitor;
class property;

/// A loose container of properties; aggregates property notification
/// i.e. if a contained property changes, so the property_container
/// will notify it has changed
class OPENPLUGINLIB_DECLSPEC iproperty_container
{
public:
	virtual ~iproperty_container() {};

    // property container
    /// locate a property with key \a k; returns a NULL_PROPERTY if not found
    virtual property get_property_with_string( const char* k ) const = 0;

    /// locate a property with key \a k; returns a NULL_PROPERTY if not found
    virtual property get_property_with_key( const key& k ) const = 0;

    /// return a list of all known property keys
    virtual key_vector get_keys() const = 0;

    /// add a property to the collection
    virtual void append( const property& ) = 0;

    /// remove a property from the collection
    virtual void remove( const property& ) = 0;

    /// visitor interface
    virtual void accept( visitor& ) = 0;
};

} } }

#endif // PCOS_IPROPERTY_CONTAINER_H
