
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.


#ifndef PCOS_PROPERTY_CONTAINER_H
#define PCOS_PROPERTY_CONTAINER_H

#if _MSC_VER
#pragma warning ( push )
#	pragma warning ( disable:4251 )
#endif

#include <openpluginlib/pl/config.hpp>

// framework
#include <openpluginlib/pl/pcos/property.hpp>
#include <openpluginlib/pl/pcos/key.hpp>
#include <openpluginlib/pl/pcos/iclonable.hpp>
#include <openpluginlib/pl/pcos/isubject.hpp>
#include <openpluginlib/pl/pcos/iproperty_container.hpp>
#include <openpluginlib/pl/pcos/visitor.hpp>

// boost
#include <boost/shared_ptr.hpp>

// std
#include <iosfwd>

namespace olib { namespace openpluginlib { namespace pcos {

/// A loose container of properties; aggregates property notification
/// i.e. if a contained property changes, so the property_container
/// will notify it has changed
class OPENPLUGINLIB_DECLSPEC property_container : public iclonable,
						  public iproperty_container, 
						  public isubject
{
public:
    property_container();
    ~property_container();

    /// equality check; impl pointer comparison is enough
    bool operator==( const property_container& ) const;
    bool operator!=( const property_container& ) const;

    // isubject interface
    void attach( boost::shared_ptr< observer > );
    void detach( boost::shared_ptr< observer > );
    void update();
    void block( boost::shared_ptr< observer > );
    void unblock( boost::shared_ptr< observer > );

    // iproperty_container interface
    property get_property_with_string( const char* k ) const;
    property get_property_with_key( const key& k ) const;
    key_vector get_keys() const;
    void append( const property& );
    void remove( const property& );
    void accept( visitor& );

    // iclonable interface
    property_container* clone() const;

private:
    class property_container_impl;
    boost::shared_ptr< property_container_impl > impl_;
};

OPENPLUGINLIB_DECLSPEC std::ostream& operator<<( std::ostream&, const property_container& );

template < typename T > void assign( property_container dest, property_container &src, const key &key )
{
	property original = src.get_property_with_key( key );
	property target = dest.get_property_with_key( key );
	if( original.valid( ) && !target.valid( ) )
		dest.append( property( key ) = original.value< T >( ) );
	else if( original.valid( ) && target.valid( ) )
		target = original.value< T >( );
}

template < typename T > void assign( property_container dest, property_container &src, const key &key, const T &default_value )
{
	property original = src.get_property_with_key( key );
	property target = dest.get_property_with_key( key );
	if( original.valid( ) && !target.valid( ) )
		dest.append( property( key ) = original.value< T >( ) );
	else if( original.valid( ) && target.valid( ) )
		target = original.value< T >( );
	else
		dest.append( property( key ) = default_value );
}

template < typename T > void assign( property_container dest, const key &key, const T &default_value )
{
	property target = dest.get_property_with_key( key );
	if( !target.valid( ) )
		dest.append( property( key ) = default_value );
	else
		target = default_value;
}

template < typename T > T value( const property_container &dest, const key &key, const T &default_value )
{
	T result = default_value;
	property target = dest.get_property_with_key( key );
	if( target.valid( ) )
		result = target.value< T >( );
	return result;
}

} } }

#if _MSC_VER
#pragma warning ( pop )
#endif

#endif // PCOS_PROPERTY_CONTAINER_H
