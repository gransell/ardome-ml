/* -*- mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*- */
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <sstream>
#include <algorithm>

#include <openpluginlib/pl/pcos/property.hpp>
#include <openpluginlib/pl/pcos/subject.hpp>
#include <openpluginlib/pl/pcos/property_container.hpp>
#include <openpluginlib/pl/pcos/any.hpp>

#include <openpluginlib/pl/string.hpp>
#include <openpluginlib/pl/utf8_utils.hpp>

#include <openimagelib/il/basic_image.hpp>
//#include <openmedialib/ml/frame.hpp>
//#include <openmedialib/ml/input.hpp>
//#include <openmedialib/ml/store.hpp>
#include <openimagelib/il/basic_image.hpp>

#include <boost/cstdint.hpp>

namespace olib
{
    namespace openmedialib
    {
        namespace ml
        {
            class frame_type;
            class store_type;
            class input_type;

            typedef boost::shared_ptr< frame_type > frame_type_ptr;
            typedef boost::shared_ptr< store_type > store_type_ptr;
            typedef boost::shared_ptr< input_type > input_type_ptr;
        }
    }
}

namespace il = olib::openimagelib::il;
namespace ml = olib::openmedialib::ml;

namespace olib { namespace openpluginlib { namespace pcos {

class property::property_impl
{
public:
	property_impl( const key& k )
		: key_( k ),
		  always_notify( false )
		{}

	/// note: clone doesn't copy across the subject i.e. if you clone a
	/// property, you wouldn't (I don't think) expect the observers to 
	/// notice if the clone changes...
	property_impl* clone() const
	{
		property_impl* result = new property_impl( key_ );
		result->value = value;
		result->always_notify = always_notify;
		result->container_ = container_;

		return result;
	}

	key key_;
	any value;
	bool always_notify;
	subject subject_;
	property_container container_;
};

property property::NULL_PROPERTY( key::from_string( "null-property" ) );

property::property()
{
	*this = NULL_PROPERTY;
}

property::property( const key& k )
	: impl_( new property_impl( k ) )
{}

property::~property()
{}

void property::attach( boost::shared_ptr< observer > obs )
{
	impl_->subject_.attach( obs );
}

void property::detach( boost::shared_ptr< observer > obs )
{
	impl_->subject_.attach( obs );
}

void property::update()
{
	impl_->subject_.update();
}

void property::block( boost::shared_ptr< observer > obs )
{
	impl_->subject_.block( obs );
}

void property::unblock( boost::shared_ptr< observer > obs )
{
	impl_->subject_.unblock( obs );
}

property property::get_property_with_string( const char* k ) const
{
	return impl_->container_.get_property_with_string( k );
}

property property::get_property_with_key( const key& k ) const
{
	return impl_->container_.get_property_with_key( k );
}

key_vector property::get_keys() const
{
	return impl_->container_.get_keys();
}

void property::append( const property& p )
{
	impl_->container_.append( p );
}

void property::remove( const property& p )
{
	impl_->container_.remove( p );
}

property* property::clone() const
{
	property* result = new property( get_key() );
	result->impl_.reset( impl_->clone() );

	return result;
}

key property::get_key() const
{
	return impl_->key_;
}

property& property::operator=( const any& v )
{
	if ( !impl_->value.empty() && 
		 v.type() != impl_->value.type() )
	{
		/// \todo emit warning about trying to change type
		/// \todo consider throwing here
		return *this;
	}

	if ( !impl_->always_notify && v == impl_->value )
	{
		return *this;
	}

	/// \todo check for ranges here
	impl_->value = v;
	update();

	return *this;
}

property& property::operator=( const property& p )
{
	impl_ = p.impl_;
	if ( !impl_.get() )
		return *this;

	update();

	return *this;
}

bool property::operator==( const property& rhs ) const
{
	return impl_ == rhs.impl_;
}

bool property::operator!=( const property& rhs ) const
{
	return !operator==( rhs );
}

template < typename T > void property::set( const T& v )
{
	operator=( any( v ) );
}

void property::set_from_string( const wstring& str )
{
	operator=( impl_->value.from_string( str ) );
}

void property::set_from_property( const property& p )
{
	impl_->value = p.impl_->value;
	update( );
}

template < typename T > T property::value() const
{
	if ( impl_->value.empty() )
	{
		return T();
	}

	return any_cast< T >( impl_->value );
}

template < typename T > T* property::pointer( ) const
{
	if ( impl_->value.empty( ) )
		return ( T* ) 0;

	return any_cast< T >( &impl_->value );
}
	
template < typename T > bool property::is_a() const
{
	return impl_->value.type() == typeid( T );
}

void property::set_always_notify( bool b )
{
	impl_->always_notify = b;
}

void property::accept( visitor& v )
{
	v.visit_property( *this );
	v.visit_property_container( impl_->container_ );
}

bool property::valid() const
{ 
	return *this != NULL_PROPERTY; 
}

template OPENPLUGINLIB_DECLSPEC int property::value< int >() const;
template OPENPLUGINLIB_DECLSPEC unsigned int property::value< unsigned int >() const;
template OPENPLUGINLIB_DECLSPEC boost::int64_t property::value< boost::int64_t >() const;
template OPENPLUGINLIB_DECLSPEC boost::uint64_t property::value< boost::uint64_t >() const;
template OPENPLUGINLIB_DECLSPEC float property::value< float >() const;
template OPENPLUGINLIB_DECLSPEC double property::value< double >() const;
template OPENPLUGINLIB_DECLSPEC int_list property::value< int_list >() const;
template OPENPLUGINLIB_DECLSPEC uint_list property::value< uint_list >() const;
template OPENPLUGINLIB_DECLSPEC double_list property::value< double_list >() const;
template OPENPLUGINLIB_DECLSPEC bool property::value< bool >() const;
template OPENPLUGINLIB_DECLSPEC string property::value< string >() const;
template OPENPLUGINLIB_DECLSPEC void* property::value< void* >() const;
template OPENPLUGINLIB_DECLSPEC int_vec property::value< int_vec >() const;
template OPENPLUGINLIB_DECLSPEC double_vec property::value< double_vec >() const;
template OPENPLUGINLIB_DECLSPEC string_vec property::value< string_vec >() const;
template OPENPLUGINLIB_DECLSPEC void_vec property::value< void_vec >() const;

template OPENPLUGINLIB_DECLSPEC wstring* property::pointer< wstring >() const;
template OPENPLUGINLIB_DECLSPEC string* property::pointer< string >() const;
template OPENPLUGINLIB_DECLSPEC wstring_vec* property::pointer< wstring_vec >() const;
template OPENPLUGINLIB_DECLSPEC string_vec* property::pointer< string_vec >() const;

template OPENPLUGINLIB_DECLSPEC void property::set< int >( const int& );
template OPENPLUGINLIB_DECLSPEC void property::set< unsigned int >( const unsigned int& );
template OPENPLUGINLIB_DECLSPEC void property::set< boost::int64_t >( const boost::int64_t& );
template OPENPLUGINLIB_DECLSPEC void property::set< boost::uint64_t >( const boost::uint64_t& );
template OPENPLUGINLIB_DECLSPEC void property::set< double >( const double& );
template OPENPLUGINLIB_DECLSPEC void property::set< int_list >( const int_list& );
template OPENPLUGINLIB_DECLSPEC void property::set< uint_list >( const uint_list& );
template OPENPLUGINLIB_DECLSPEC void property::set< double_list >( const double_list& );
template OPENPLUGINLIB_DECLSPEC void property::set< bool >( const bool& );

template OPENPLUGINLIB_DECLSPEC bool property::is_a< int >() const;
template OPENPLUGINLIB_DECLSPEC bool property::is_a< unsigned int >() const;
template OPENPLUGINLIB_DECLSPEC bool property::is_a< boost::int64_t >() const;
template OPENPLUGINLIB_DECLSPEC bool property::is_a< boost::uint64_t >() const;
template OPENPLUGINLIB_DECLSPEC bool property::is_a< double >() const;
template OPENPLUGINLIB_DECLSPEC bool property::is_a< int_list >() const;
template OPENPLUGINLIB_DECLSPEC bool property::is_a< uint_list >() const;
template OPENPLUGINLIB_DECLSPEC bool property::is_a< double_list >() const;
template OPENPLUGINLIB_DECLSPEC bool property::is_a< bool >() const;
template OPENPLUGINLIB_DECLSPEC bool property::is_a< string >() const;
template OPENPLUGINLIB_DECLSPEC bool property::is_a< void* >() const;
template OPENPLUGINLIB_DECLSPEC bool property::is_a< string_vec >() const;
template OPENPLUGINLIB_DECLSPEC bool property::is_a< int_vec >() const;
template OPENPLUGINLIB_DECLSPEC bool property::is_a< double_vec >() const;
template OPENPLUGINLIB_DECLSPEC bool property::is_a< void_vec >() const;

template OPENPLUGINLIB_DECLSPEC wstring property::value< wstring >() const;
template OPENPLUGINLIB_DECLSPEC wstring_list property::value< wstring_list >() const;
template OPENPLUGINLIB_DECLSPEC string_list property::value< string_list >() const;

template OPENPLUGINLIB_DECLSPEC void property::set< wstring >( const wstring& );
template OPENPLUGINLIB_DECLSPEC void property::set< string >( const string& );
template OPENPLUGINLIB_DECLSPEC void property::set< wstring_list >( const wstring_list& );
template OPENPLUGINLIB_DECLSPEC void property::set< string_list >( const string_list& );

template OPENPLUGINLIB_DECLSPEC bool property::is_a< wstring >() const;
template OPENPLUGINLIB_DECLSPEC bool property::is_a< wstring_list >() const;
template OPENPLUGINLIB_DECLSPEC bool property::is_a< string_list >() const;

template OPENPLUGINLIB_DECLSPEC il::image_type_ptr property::value< il::image_type_ptr >() const;
template OPENPLUGINLIB_DECLSPEC void property::set< il::image_type_ptr >( const il::image_type_ptr& );
template OPENPLUGINLIB_DECLSPEC bool property::is_a< il::image_type_ptr >() const;

template OPENPLUGINLIB_DECLSPEC ml::store_type_ptr property::value< ml::store_type_ptr >() const;
template OPENPLUGINLIB_DECLSPEC void property::set< ml::store_type_ptr >( const ml::store_type_ptr& );
template OPENPLUGINLIB_DECLSPEC bool property::is_a< ml::store_type_ptr >() const;

template OPENPLUGINLIB_DECLSPEC ml::frame_type_ptr property::value< ml::frame_type_ptr >() const;
template OPENPLUGINLIB_DECLSPEC void property::set< ml::frame_type_ptr >( const ml::frame_type_ptr& );
template OPENPLUGINLIB_DECLSPEC bool property::is_a< ml::frame_type_ptr >() const;

template OPENPLUGINLIB_DECLSPEC ml::input_type_ptr property::value< ml::input_type_ptr >() const;
template OPENPLUGINLIB_DECLSPEC void property::set< ml::input_type_ptr >( const ml::input_type_ptr& );
template OPENPLUGINLIB_DECLSPEC bool property::is_a< ml::input_type_ptr >() const;


// implementation of parsing code for pcos::any

template < typename T > T parse_string( const wstring& str )
{
	std::wistringstream istrm( str.c_str( ) );
	T v;
	istrm >> v;
	return v;	
}

template <> OPENPLUGINLIB_DECLSPEC bool parse_string( const wstring& str )
{
	if ( str.find( L"true" ) != wstring::npos )
	{
		return true;
	}

	return false;
}

template <> OPENPLUGINLIB_DECLSPEC string parse_string( const wstring& str )
{
	/// the best that can be hoped for in this case is straight decimation yields something
	return to_string( str );
}

template <> OPENPLUGINLIB_DECLSPEC wstring parse_string( const wstring& str )
{
	return str;
}

template OPENPLUGINLIB_DECLSPEC int parse_string( const wstring& str );
template OPENPLUGINLIB_DECLSPEC float parse_string( const wstring& str );
template OPENPLUGINLIB_DECLSPEC double parse_string( const wstring& str );
template OPENPLUGINLIB_DECLSPEC unsigned int parse_string( const wstring& str );
template OPENPLUGINLIB_DECLSPEC boost::int64_t parse_string( const wstring& str );
template OPENPLUGINLIB_DECLSPEC boost::uint64_t parse_string( const wstring& str );

/// split lists on colons
template < typename RESULT_T > RESULT_T split_list( const wstring& str )
{
	static const wchar_t SEPARATOR = L':';

	RESULT_T result;
	size_t last_pos = 0;
	size_t current_pos = str.find( SEPARATOR );
	while ( current_pos != wstring::npos )
	{
		result.push_back( parse_string< typename RESULT_T::value_type >( wstring( str, last_pos, current_pos - last_pos ) ) );

		last_pos = ++current_pos;
		current_pos = str.find( SEPARATOR, last_pos );
	}
	result.push_back( parse_string< typename RESULT_T::value_type >( wstring( str, last_pos, current_pos ) ) );

	return result;
}

template <> OPENPLUGINLIB_DECLSPEC wstring_list parse_string( const wstring& str )
{
	return split_list< wstring_list >( str );
}

template <> OPENPLUGINLIB_DECLSPEC string_list parse_string( const wstring& str )
{
	return split_list< string_list >( str );
}

template <> OPENPLUGINLIB_DECLSPEC int_list parse_string( const wstring& str )
{
	return split_list< int_list >( str );
}

template <> OPENPLUGINLIB_DECLSPEC uint_list parse_string( const wstring& str )
{
	return split_list< uint_list >( str );
}

template <> OPENPLUGINLIB_DECLSPEC double_list parse_string( const wstring& str )
{
	return split_list< double_list >( str );
}

template <> OPENPLUGINLIB_DECLSPEC double_vec parse_string( const wstring& )
{
	return double_vec( );
}

template <> OPENPLUGINLIB_DECLSPEC int_vec parse_string( const wstring& )
{
	return int_vec( );
}

template <> OPENPLUGINLIB_DECLSPEC string_vec parse_string( const wstring& str )
{
	return string_vec( 1, to_string( str ) );
}

template <> OPENPLUGINLIB_DECLSPEC wstring_vec parse_string( const wstring& str )
{
	return wstring_vec( 1, str );
}

template <> OPENPLUGINLIB_DECLSPEC void* parse_string( const wstring& )
{
	return 0;
}

template <> OPENPLUGINLIB_DECLSPEC il::image_type_ptr parse_string( const wstring& )
{
	return il::image_type_ptr();
}

template <> OPENPLUGINLIB_DECLSPEC ml::frame_type_ptr parse_string( const wstring& )
{
    return ml::frame_type_ptr();
}

template <> OPENPLUGINLIB_DECLSPEC ml::store_type_ptr parse_string( const wstring& )
{
    return ml::store_type_ptr();
}

template <> OPENPLUGINLIB_DECLSPEC ml::input_type_ptr parse_string( const wstring& )
{
    return ml::input_type_ptr();
}

template <> OPENPLUGINLIB_DECLSPEC void_vec parse_string( const wstring& )
{
	return void_vec( );
}

std::ostream& operator<<( std::ostream& os , const property& p )
{
	os << p.get_key() << ": ";
	if ( p.is_a< int >() ) os << p.value< int >();
	else if ( p.is_a< unsigned int >() ) os << p.value< unsigned int >();
	else if ( p.is_a< boost::int64_t >() ) os << p.value< boost::int64_t >();
	else if ( p.is_a< boost::uint64_t >() ) os << p.value< boost::uint64_t >();
	else if ( p.is_a< double >() ) os << p.value< double >();
	else if ( p.is_a< bool >() ) os << p.value< bool >();
	else if ( p.is_a< string >() ) os << p.value< string >();
	else if ( p.is_a< wstring >() ) os << to_string( p.value< wstring >() );
	else 
		os << "*** write type output! ***";

	return os;
}

} } }
