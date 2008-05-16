
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef OPL_PARSER_ACTION_INC_
#define OPL_PARSER_ACTION_INC_

#if _MSC_VER >= 1400
#include <hash_map>
#endif

#include <map>

#ifndef BOOST_FILESYSTEM_DYN_LINK 
    #define BOOST_FILESYSTEM_DYN_LINK 
#endif

#include <boost/filesystem/path.hpp>

// forward declarations
#ifdef WIN32
struct MSXML2::ISAXAttributes;
#else
#include <libxml/parser.h>
#endif // WIN32

#include <openpluginlib/pl/openplugin.hpp>
#include <openpluginlib/pl/string.hpp>

namespace olib { namespace openpluginlib { namespace actions {

class opl_parser_action;

typedef bool ( *opl_dispatcher )( opl_parser_action& pa );

class opl_parser_action
{
public:
	typedef wstring 				key_type;
	typedef boost::filesystem::path	path;
	
#if _MSC_VER >= 1400
	typedef stdext::hash_multimap<key_type, detail::plugin_item> container;
#else
	typedef std::multimap<key_type, detail::plugin_item> container;
#endif

	typedef std::multimap<key_type, opl_dispatcher> opl_dispatcher_container;

	explicit opl_parser_action( );
	virtual ~opl_parser_action( );

	bool dispatch( const wstring& tag );

	void start( );
	void finish( );

	wstring get_libname( ) const
	{ return libname_; }
	void set_libname( const wstring& libname )
	{ libname_ = libname; }

	wstring get_current_tag( ) const
	{ return tag_; }
	void set_current_tag( const wstring& tag )
	{ tag_ = tag; }

	path get_branch_path( ) const;
	void set_branch_path( const path& branch_path );

public:
#ifdef WIN32
	void set_attrs( MSXML2::ISAXAttributes __RPC_FAR* attrs );
#else
	void set_attrs( xmlChar** attrs );
#endif

public:
#ifdef WIN32
	MSXML2::ISAXAttributes __RPC_FAR* attrs_;
#else
	xmlChar** attrs_;
#endif

	container plugins;

private:
	opl_dispatcher_container dispatch_;
	
	path branch_path_;
	wstring libname_, tag_;
};

bool default_opl_parser_action( opl_parser_action& pa );

bool olib_opl_parser_action( opl_parser_action& pa );
bool plugin_opl_parser_action( opl_parser_action& pa );

} } }

#endif
