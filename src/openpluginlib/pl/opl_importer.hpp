
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef OPL_IMPORTER_INC_
#define OPL_IMPORTER_INC_

#if _MSC_VER >= 1400
#include <hash_map>
#endif

#include <map>
#include <string>

#include <xercesc/sax2/DefaultHandler.hpp>

XERCES_CPP_NAMESPACE_USE

#ifndef BOOST_FILESYSTEM_DYN_LINK 
    #define BOOST_FILESYSTEM_DYN_LINK 
#endif
#include <boost/filesystem/path.hpp>

#include <openpluginlib/pl/openplugin.hpp>
#include <openpluginlib/pl/opl_parser_action.hpp>

namespace olib { namespace openpluginlib {

class opl_importer : 
	private DefaultHandler
{
public:
	typedef std::wstring key_type;
	
#if _MSC_VER >= 1400
	typedef stdext::hash_multimap<key_type, detail::plugin_item> container;
#else
	typedef std::multimap<key_type, detail::plugin_item> container;
#endif

	opl_importer( );
	~opl_importer( );

	void operator( )( const boost::filesystem::path& file );
	
	container plugins;
	bool auto_load;

private:

    void startElement (
        const XMLCh* const uri,
        const XMLCh* const localname,
        const XMLCh* const qname,
        const Attributes& attrs );

	void error(const SAXParseException& exc);

	actions::opl_parser_action action_;

};

} }

#endif
