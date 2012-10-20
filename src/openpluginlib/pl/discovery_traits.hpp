
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef DISCOVERY_TRAITS_INC_
#define DISCOVERY_TRAITS_INC_

namespace olib { namespace openpluginlib {

// default_query_traits assumes the merit functional to be greater<int>.
struct default_query_traits
{
	std::wstring libname( ) const
	{ return std::wstring( L"" ); }
	
	std::wstring type( ) const
	{ return std::wstring( L"" ); }
		
	std::wstring to_match( ) const
	{ return std::wstring( L"" ); }
		
	int merit( ) const
	{ return 0; }
};

struct openeffectslib_default_query_traits
	: public default_query_traits
{
	std::wstring libname( ) const
	{ return std::wstring( L"openeffectslib" ); }
};

struct openimagelib_default_query_traits
	: public default_query_traits
{
	std::wstring libname( ) const
	{ return std::wstring( L"openimagelib" ); }
};

struct openmedialib_default_query_traits
	: public default_query_traits
{
	std::wstring libname( ) const
	{ return std::wstring( L"openmedialib" ); }
};
	
struct openobjectlib_default_query_traits
	: public default_query_traits
{
	std::wstring libname( ) const
	{ return std::wstring( L"openobjectlib" ); }
};

struct openassetlib_default_query_traits
	: public default_query_traits
{
	std::wstring libname( ) const
	{ return std::wstring( L"openassetlib" ); }
};

} }

#endif
