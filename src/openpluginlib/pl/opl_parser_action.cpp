
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <algorithm>
#include <cstdio>


#include <boost/tokenizer.hpp>

#include <openpluginlib/pl/openpluginlib.hpp>
#include <openpluginlib/pl/opl_parser_action.hpp>
#include <opencorelib/cl/export_defines.hpp>
#include <opencorelib/cl/str_util.hpp>
#include "../../opencorelib/cl/xerces_sax_traverser.hpp"

namespace fs = boost::filesystem;

namespace olib { namespace openpluginlib { namespace actions {

namespace
{
	std::wstring value_from_name( opl_parser_action& pa, const std::wstring& name )
	{
		std::wstring str;
		const XMLCh *s = pa.attrs_->getValue(opencorelib::xml::from_string(name).c_str());

		if (s)
		{
			str = opencorelib::xml::to_wstring(s);
		}

		return str;
	}
	
	void vector_from_string( const std::wstring& str, std::vector<std::wstring>& values )
	{
		typedef boost::tokenizer<boost::char_separator<wchar_t>, std::wstring::const_iterator, std::wstring> wtokenizer;

		boost::char_separator<wchar_t> sep( L" ,\"" );
		wtokenizer wtok( str.begin( ), str.end( ), sep );

		std::copy( wtok.begin( ), wtok.end( ), std::back_inserter( values ) );
	}

	void regexes_from_strings( const std::vector<std::wstring> &strings, std::vector<boost::wregex>& regexes )
	{
		regexes.reserve(strings.size());
		for( size_t i = 0; i < strings.size(); ++i )
		{
			regexes.push_back(boost::wregex(strings[i], boost::wregex::extended | boost::wregex::icase));
		}
	}
	
	void add_opl_path_to_search( const opl_parser_action& pa, std::vector<std::wstring>& filenames )
	{
		namespace opl = olib::openpluginlib;
		
		typedef std::vector<std::wstring>::iterator 		 iterator;
		typedef std::vector<std::wstring>::const_iterator const_iterator;
				
		std::vector<std::wstring> filenames_copy( filenames );
		for( const_iterator I = filenames_copy.begin( ); I != filenames_copy.end( ); ++I )
		{
			fs::path tmp( olib::opencorelib::str_util::to_t_string( *I ) );

			filenames.push_back( olib::opencorelib::str_util::to_wstring( ( pa.get_opl_path().parent_path() / tmp.filename( ) ).c_str( ) ) );
			
#	ifdef WIN32
			filenames.push_back( olib::opencorelib::str_util::to_wstring( ( fs::path( olib::opencorelib::str_util::to_t_string( opl::plugins_path( ) ) / tmp.filename( ) ).c_str( ) ) ) );
#	endif
		}
	}
}

opl_parser_action::opl_parser_action( )
: auto_load_( false )
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

bool opl_parser_action::dispatch( const std::wstring& tag )
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

opl_parser_action::path opl_parser_action::get_opl_path( ) const
{ return opl_path_; }

void opl_parser_action::set_opl_path( const opl_parser_action::path& opl_path )
{ opl_path_ = opl_path; }


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
	pa.set_auto_load( value_from_name( pa, L"auto_load" ) == L"true" );
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
	item.opl_path = olib::opencorelib::str_util::to_wstring( pa.get_opl_path( ).native() );

	std::vector<std::wstring> temp;
	vector_from_string( value_from_name( pa, L"extension" ), temp );
	regexes_from_strings( temp, item.extension );
	vector_from_string( value_from_name( pa, L"filename"  ), item.filenames );

	if( !item.filenames.empty( ) )
		add_opl_path_to_search( pa, item.filenames );

	pa.plugins.insert( opl_parser_action::container::value_type( pa.get_libname( ), item ) );

	return true;
}

} } }
