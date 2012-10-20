
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef PCOS_PROPERTY_H
#define PCOS_PROPERTY_H

#include <openpluginlib/pl/config.hpp>

#include <openpluginlib/pl/pcos/isubject.hpp>
#include <openpluginlib/pl/pcos/iproperty_container.hpp>
#include <openpluginlib/pl/pcos/iclonable.hpp>
#include <openpluginlib/pl/pcos/key.hpp>
#include <openpluginlib/pl/pcos/any.hpp>
#include <openpluginlib/pl/pcos/visitor.hpp>

// boost
#include <boost/shared_ptr.hpp>

// std
#include <list>
#include <iosfwd>

namespace olib { namespace openpluginlib { namespace pcos {

class any;

/// A property class i.e. a pairing of a key to a value.
/// Properties are reference counted, and so may be passed by 
/// value if desired without much overhead.
class OPENPLUGINLIB_DECLSPEC property : public iclonable,
                                        public iproperty_container, 
                                        public isubject
{
public:
    static property NULL_PROPERTY;

    property( const key& );
    virtual ~property();

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

    // iclonable interface
    property* clone() const;

    // property
    /// allow retrieval of the key
    key get_key() const;

    /// allow direct assignment
    property& operator=( const any& );
    property& operator=( const property& );

    /// equality check; impl pointer comparison is enough
    bool operator==( const property& ) const;
    bool operator!=( const property& ) const;

    /// direct set of a value
    template < typename T > void set( const T& );

    /// allow set of a property value from a string
    /// i.e. parse string for value of appropriate type
    void set_from_string( const std::wstring& );

    /// allow set of a property value from a property
    void set_from_property( const property& );

    /// return the value; will throw a bad_any_cast if 
    /// we try to extract the wrong value type
    template < typename T > T  value() const;
    template < typename T > T* pointer() const;
    
    /// query the type held by the property
    template < typename T > bool is_a() const;

    /// always notify any observers on an assignment
    /// (by default, notification only happens if the assigned
    /// value and the current value are different)
    void set_always_notify( bool );

    /// visitor interface
    void accept( visitor& );

    /// Check if we're usable
    bool valid() const;

private:
    property();

    class property_impl;
    boost::shared_ptr< property_impl > impl_;
};

OPENPLUGINLIB_DECLSPEC std::ostream& operator<<( std::ostream&, const property& );


// Sets up getter and setter for a certain property with predefined data type.
OPENPLUGINLIB_DECLSPEC void prop_enforce( bool ok, const char* msg );
OPENPLUGINLIB_DECLSPEC void prop_err( const char* msg );

/*
 * The following macro definitions works as helpers, defining getters and
 * setters (and "has_"-functions) for properties.
 * You'll probably only want to DEFINE_PROPERTY.
 *
 * DEFINE_PROPERTY
 * Defines a property with get_, set_ and has_ functions for the same name as
 * the property per se, and with the same type.
 *
 * DEFINE_PROPERTY_AS
 * Defines a property with a different getter/setter name than the real
 * property name, but with the same type.
 *
 * DEFINE_PROPERTY_TYPED_AS
 * Defines a property with a possibly different getter and setter name than
 * the real property and with a different type in code than the inner property
 * type. You'll likely not want to use this.
 *
 */
#define DEFINE_PROPERTY_TYPED_AS( Type, RealType, Name, CodeName ) \
	bool has_##CodeName( ) { \
		namespace pcos = olib::openpluginlib::pcos; \
		pcos::property prop = properties_.get_property_with_string( #Name ); \
		if ( prop.valid( ) && !prop.is_a< RealType >( ) ) { \
			pcos::prop_err( "Property \"" #Name "\" is of conflicting type!" ); \
			return false; \
		} \
		return prop.valid( ); \
	} \
	Type get_##CodeName( ) { \
		namespace pcos = olib::openpluginlib::pcos; \
		pcos::property prop = properties_.get_property_with_string( #Name ); \
		pcos::prop_enforce( prop.valid( ), "Property \"" #Name "\" not defined" ); \
		pcos::prop_enforce( prop.is_a< RealType >( ), "Property \"" #Name "\" is of conflicting type!" ); \
		return static_cast< Type >( prop.value< RealType >( ) ); \
	} \
	void set_##CodeName( const Type& val ) { \
		namespace pcos = olib::openpluginlib::pcos; \
		const pcos::key key = pcos::key::from_string( #Name ); \
		pcos::property prop( key ); \
		properties_.append( prop = static_cast< RealType >( val ) ); \
	}

#define DEFINE_PROPERTY_AS( Type, Name, CodeName ) \
	DEFINE_PROPERTY_TYPED_AS( Type, Type, Name, CodeName )

#define DEFINE_PROPERTY( Type, Name ) \
	DEFINE_PROPERTY_AS( Type, Name, Name )


} } }

#endif // PCOS_PROPERTY_H
