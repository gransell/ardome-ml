
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <algorithm>
#include <cstdio>

#ifdef WIN32
#import <msxml4.dll> raw_interfaces_only
using namespace MSXML2;
#endif // WIN32

#include <boost/tokenizer.hpp>

#include <openpluginlib/pl/openpluginlib.hpp>
#include <openpluginlib/pl/utf8_utils.hpp>
#include <openpluginlib/pl/opl_parser_action.hpp>

namespace fs = boost::filesystem;

namespace olib { namespace openpluginlib { namespace actions {

namespace
{
#ifndef WIN32
	bool libxml2_value_from_name( const wstring& name, xmlChar** attributes, wstring& value )
	{
		if( !attributes ) return false;

		xmlChar** begin = attributes;
		while( *begin )
		{
			if( name == to_wstring( reinterpret_cast<char*>( *begin ) ) )
			{
				value = to_wstring( reinterpret_cast<char*>( *( begin + 1 ) ) );
				
				return true;
			}

			begin += 2;
		}
		
		return false;
	}
#endif

	wstring value_from_name( opl_parser_action& pa, const wstring& name )
	{
#ifdef WIN32
		wchar_t* value = NULL;
		int length;

		if( SUCCEEDED(	pa.attrs_->getValueFromName(
						( unsigned short*  ) L"", 0,
						( unsigned short*  ) name.c_str( ), ( int ) name.size( ),
						( unsigned short** ) &value, &length ) ) )
		{
			return wstring( value, length );
		}
#else
		wstring str;
		if( libxml2_value_from_name( name, pa.attrs_, str ) )
			return str;
#endif
		
		return wstring( L"" );
	}
	
	void vector_from_string( const wstring& str, std::vector<wstring>& values )
	{
		typedef boost::tokenizer<boost::char_separator<wchar_t>, wstring::const_iterator, wstring> wtokenizer;

		boost::char_separator<wchar_t> sep( L" ,\"" );
		wtokenizer wtok( str.begin( ), str.end( ), sep );

		std::copy( wtok.begin( ), wtok.end( ), std::back_inserter( values ) );
	}

	void regexes_from_strings( const std::vector<wstring> &strings, std::vector<boost::wregex>& regexes )
	{
		regexes.reserve(strings.size());
		for( int i = 0; i < strings.size(); ++i )
		{
			regexes.push_back(boost::wregex(strings[i], boost::wregex::extended | boost::wregex::icase));
		}
	}
	
	void add_opl_path_to_search( const opl_parser_action& pa, std::vector<wstring>& filename )
	{
		namespace opl = olib::openpluginlib;
		
		typedef std::vector<wstring>::iterator 		 iterator;
		typedef std::vector<wstring>::const_iterator const_iterator;
				
		std::vector<wstring> filename_copy( filename );
		for( const_iterator I = filename_copy.begin( ); I != filename_copy.end( ); ++I )
		{
			fs::path tmp( to_string( *I ).c_str( ), fs::native );

			filename.push_back( to_wstring( ( pa.get_branch_path( ) / tmp.leaf( ) ).string( ).c_str( ) ) );
			
#	ifdef WIN32
			filename.push_back( to_wstring( ( fs::path( opl::plugins_path( ).c_str( ), fs::native ) / tmp.leaf( ) ).string( ).c_str( ) ) );
#	endif
		}
	}
}

opl_parser_action::opl_parser_action( )
{
	dispatch_.insert( opl_dispatcher_container::value_type( L"openlibraries",		default_opl_parser_action ) );
	dispatch_.insert( opl_dispatcher_container::value_type( L"head",				default_opl_parser_action ) );
	dispatch_.insert( opl_dispatcher_container::value_type( L"meta",				default_opl_parser_action ) );
	dispatch_.insert( opl_dispatcher_container::value_type( L"openassetlib",		olib_opl_parser_action ) );
	dispatch_.insert( opl_dispatcher_container::value_type( L"openeffectslib",		olib_opl_parser_action ) );
	dispatch_.insert( opl_dispatcher_container::value_type( L"openimagelib",		olib_opl_parser_action ) );
	dispatch_.insert( opl_dispatcher_container::value_type( L"openmedialib",		olib_opl_parser_action ) );
	dispatch_.insert( opl_dispatcher_container::value_type( L"opennetworklib",		olib_opl_parser_action ) );
	dispatch_.insert( opl_dispatcher_container::value_type( L"openobjectlib",		olib_opl_parser_action ) );
	dispatch_.insert( opl_dispatcher_container::value_type( L"plugin",				plugin_opl_parser_action ) );
}

opl_parser_action::~opl_parser_action( )
{
}

bool opl_parser_action::dispatch( const wstring& tag )
{
	typedef opl_dispatcher_container::const_iterator const_iterator;
	
	const_iterator I = dispatch_.find( tag );
	if( I != dispatch_.end( ) ) 
	{
		set_current_tag( tag );
		
		return I->second( *this );
	}

	assert( 0 && L"opl_parser_action::dispatch invalid xml node." );
	
	return false;
}

void opl_parser_action::start( )
{
}

void opl_parser_action::finish( )
{
}

#ifdef WIN32
void opl_parser_action::set_attrs( ISAXAttributes __RPC_FAR* attrs )
#else
void opl_parser_action::set_attrs( xmlChar** attrs )
#endif
{
	attrs_ = attrs;
}

opl_parser_action::path opl_parser_action::get_branch_path( ) const
{ return branch_path_; }

void opl_parser_action::set_branch_path( const opl_parser_action::path& branch_path )
{ branch_path_ = branch_path; }

bool default_opl_parser_action( opl_parser_action& /*pa*/ )
{
	return true;
}

bool openlibraries_opl_parser_action( opl_parser_action& /*pa*/ )
{
	return true;
}

bool olib_opl_parser_action( opl_parser_action& pa )
{
	pa.set_libname( pa.get_current_tag( ) );

	return true;
}

bool plugin_opl_parser_action( opl_parser_action& pa )
{
	detail::plugin_item item;

	item.name		= value_from_name( pa, L"name" );
	item.type		= value_from_name( pa, L"type" );
	item.mime		= value_from_name( pa, L"mime" );
	item.category	= value_from_name( pa, L"category" );
	item.in_filter	= value_from_name( pa, L"in_filter" );
	item.out_filter	= value_from_name( pa, L"out_filter" );
	item.libname	= pa.get_libname( );
	item.merit		= static_cast<int>( wcstol( value_from_name( pa, L"merit" ).c_str( ), 0, 10 ) );

	std::vector<wstring> temp;
	vector_from_string( value_from_name( pa, L"extension" ), temp );
	regexes_from_strings( temp, item.extension );
	vector_from_string( value_from_name( pa, L"filename"  ), item.filename );

	if( !item.filename.empty( ) )
		add_opl_path_to_search( pa, item.filename );

	pa.plugins.insert( opl_parser_action::container::value_type( pa.get_libname( ), item ) );

	return true;
}

} } }
