
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <cassert>
#include <iostream>
#include <algorithm>
#include <iterator>

#include <openpluginlib/pl/pcos/property.hpp>
#include <openpluginlib/pl/pcos/observer.hpp>
#include <openpluginlib/pl/string.hpp>

#include <openpluginlib/pl/utf8_utils.hpp>
#include <openimagelib/il/openimagelib_plugin.hpp>
#include <boost/filesystem/operations.hpp>

namespace opl = olib::openpluginlib;
namespace pcos = olib::openpluginlib::pcos;
namespace il = olib::openmedialib::ml;
namespace fs = boost::filesystem;

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

// Query structure used
struct il_query_traits : public opl::default_query_traits
{
	il_query_traits( const opl::wstring& filename, const opl::wstring &type )
		: filename_( filename )
		, type_( type )
	{ }
		
	opl::wstring to_match( ) const
	{ return filename_; }

	opl::wstring libname( ) const
	{ return opl::wstring( L"openimagelib" ); }

	opl::wstring type( ) const
	{ return opl::wstring( type_ ); }
	
	const opl::wstring filename_;
	const opl::wstring type_;
};

typedef opl::discovery<il_query_traits> discovery;

int main( int argc, char* argv[] )
{
    // check the key value
    {
        pcos::property p1( pcos::key::from_string( "foobar" ) );
        assert( p1.get_key() == pcos::key::from_string( "foobar" ) );
    }

    {
        // assignment from primitive
        pcos::property p1( pcos::key::from_string( "string-key" ) );
        p1 = opl::wstring( L"hello, world" );

        pcos::property p2( pcos::key::from_string( "double-key" ) );
        p2 = 3.141;

        pcos::property p3( pcos::key::from_string( "int-key" ) );
        p3 = 3;

        // assignment to another property
        pcos::property p1dash( p1 );
        pcos::property p2dash( p2 );
        pcos::property p3dash( p3 );

        // check types
        assert( p1.is_a< opl::wstring >() );
        assert( !p1.is_a< double >() );
        assert( !p1.is_a< int >() );

        assert( !p2.is_a< opl::wstring >() );
        assert( p2.is_a< double >() );
        assert( !p2.is_a< int >() );

        assert( !p3.is_a< opl::wstring >() );
        assert( !p3.is_a< double >() );
        assert( p3.is_a< int >() );
            
        // check values
        assert( p1.value< opl::wstring >() == opl::wstring( L"hello, world" ) );
        assert( p2.value< double >() == 3.141 );
        assert( p3.value< int >() == 3 );
    }

    // clone
    {
	pcos::property p1( pcos::key::from_string( "foo" ) );
	p1 = 3.141;

	pcos::property* p2 = p1.clone();
	*p2 = 2.171;

	assert( p1.is_a< double >() );
	assert( p2->is_a< double >() );
	assert( p1.value< double >() == 3.141 );
	assert( p2->value< double >() == 2.171 );
	assert( p1 != *p2 );

	boost::shared_ptr< counting_observer > obs( new counting_observer );
	p1.attach( obs );
	*p2 = 1.111;
	assert( obs->count == 0 );
	p1 = 2.222;
	assert( obs->count == 1 );
    }

    // subject/observer
    {
        boost::shared_ptr< counting_observer > obs( new counting_observer );
        pcos::property p( pcos::key::from_string( "string-key" ) );
        p.attach( obs );

        p = opl::wstring( L"hello, world" );
        assert( obs->count == 1 );

        pcos::property p2( pcos::key::from_string( "string2-key" ) );
        p2 = opl::wstring( L"goodbye, cruel world" );
        p = p2;
        assert( obs->count == 1 );
    }

    // assignment from string
    {
	pcos::property p_int( pcos::key::from_string( "int_key" ) );
	p_int = 1;
	p_int.set_from_string( L"2" );
	assert( p_int.value< int >() == 2 );

	pcos::property p_double( pcos::key::from_string( "double_key" ) );
	p_double = 1.0;
	p_double.set_from_string( L"3.141" );
	assert( p_double.value< double >() == 3.141 );

	pcos::property p_bool( pcos::key::from_string( "bool_key" ) );
	p_bool = false;
	p_bool.set_from_string( L"true" );
	assert( p_bool.value< bool >() == true );

	pcos::property p_string( pcos::key::from_string( "string_key" ) );
	p_string = opl::string( "" );
	p_string.set_from_string( L"foobar" );
	assert( p_string.value< opl::string >() == "foobar" );

	pcos::property p_wstring( pcos::key::from_string( "wstring_key" ) );
	p_wstring = opl::wstring( L"" );
	p_wstring.set_from_string( L"foobar" );
	assert( p_wstring.value< opl::wstring >() == L"foobar" );

	pcos::property p_stringlist( pcos::key::from_string( "stringlist_key" ) );
	p_stringlist = opl::string_list();
	p_stringlist.set_from_string( L"foo:bar:zot" );
	
	opl::string_list r;
	r.push_back( "foo" );
	r.push_back( "bar" );
	r.push_back( "zot" );

	opl::string_list stored( p_stringlist.value< opl::string_list >() );
// 	std::copy( r.begin(), r.end(), std::ostream_iterator< opl::string >( std::cout, "\n" ) );
// 	std::copy( stored.begin(), stored.end(), std::ostream_iterator< opl::string >( std::cout, "\n" ) );

	assert( stored == r );
	assert( stored.size() == 3 );

	pcos::property p_wstringlist( pcos::key::from_string( "wstringlist_key" ) );
	p_wstringlist = opl::wstring_list();
	p_wstringlist.set_from_string( L"foo:bar:zot" );
	
	opl::wstring_list wr;
	wr.push_back( L"foo" );
	wr.push_back( L"bar" );
	wr.push_back( L"zot" );

	opl::wstring_list wstored( p_wstringlist.value< opl::wstring_list >() );
// 	std::copy( wr.begin(), wr.end(), std::ostream_iterator< opl::wstring >( std::wcout, "\n" ) );
// 	std::copy( wstored.begin(), wstored.end(), std::ostream_iterator< opl::wstring >( std::wcout, "\n" ) );

	assert( wstored == wr );
	assert( wstored.size() == 3 );

	pcos::property p_int_list( pcos::key::from_string( "int_list_key" ) );
	p_int_list = pcos::int_list();
	p_int_list.set_from_string( L"1:2:3" );
	
	pcos::int_list ir;
	ir.push_back( 1 );
	ir.push_back( 2 );
	ir.push_back( 3 );

	pcos::int_list istored( p_int_list.value< pcos::int_list >() );
// 	std::copy( ir.begin(), ir.end(), std::ostream_iterator< int >( std::cout, "\n" ) );
// 	std::copy( istored.begin(), istored.end(), std::ostream_iterator< int >( std::cout, "\n" ) );

	assert( istored == ir );
	assert( istored.size() == 3 );

	pcos::property p_double_list( pcos::key::from_string( "double_list_key" ) );
	p_double_list = pcos::double_list();
	p_double_list.set_from_string( L"1.1:2.2:3.3" );
	
	pcos::double_list dr;
	dr.push_back( 1.1 );
	dr.push_back( 2.2 );
	dr.push_back( 3.3 );

	pcos::double_list dstored( p_double_list.value< pcos::double_list >() );
// 	std::copy( dr.begin(), dr.end(), std::ostream_iterator< double >( std::cout, "\n" ) );
// 	std::copy( dstored.begin(), dstored.end(), std::ostream_iterator< double >( std::cout, "\n" ) );

	assert( dstored == dr );
	assert( dstored.size() == 3 );

    }

    // image type
    {
	opl::init();
	assert( argc == 2 );
	il_query_traits query( opl::to_wstring( argv[1] ), L"input" );
	discovery plugins( query );
	boost::shared_ptr < il::openimagelib_plugin > plug_;
	assert( plugins.size() );
	
	discovery::const_iterator it = plugins.begin( );
	plug_ = boost::shared_dynamic_cast<il::openimagelib_plugin>( it->create_plugin( "" ) );

	ml::image_type_ptr im = plug_->load( fs::path( argv[1], fs::native ) );

	pcos::property p_image_type( pcos::key::from_string( "image" ) );
	p_image_type = im;
	assert( p_image_type.is_a< ml::image_type_ptr >() );
	assert( p_image_type.value< ml::image_type_ptr >() == im );
    }

    return 0;
}
