
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005 Visual Media FX Ltd.
// Released under the LGPL.
// For more information, see http://www.openobjectlib.org.

#include <openpluginlib/pl/utf8_utils.hpp>
#include <openpluginlib/pl/content_handler_libxml.hpp>

namespace
{
#ifdef __cplusplus
extern "C" {
#endif

	// callbacks to interface with libxml2 sax interface.
	void opl_startDocument( void* user_data )
	{ }

	void opl_endDocument( void* user_data )
	{ }

	void opl_startElement( void* user_data, const xmlChar* name, const xmlChar** attrs )
	{
		namespace opl = olib::openpluginlib;
		
		opl::actions::opl_parser_action* action = static_cast<opl::actions::opl_parser_action*>( user_data );

		action->set_attrs( const_cast<xmlChar**>( attrs ) );
		action->dispatch( opl::to_wstring( ( const char* ) BAD_CAST name ) );
	}

	void opl_endElement( void* user_data, const xmlChar* name )
	{
	}

#ifdef __cplusplus
}
#endif
}

namespace olib { namespace openpluginlib {

//
content_handler_libxml::content_handler_libxml( )
{
	memset( &handler_, 0, sizeof( xmlSAXHandler ) );

	handler_.startDocument 	= opl_startDocument;
	handler_.endDocument 	= opl_endDocument;
	handler_.startElement 	= opl_startElement;
	handler_.endElement 	= opl_endElement;
}

content_handler_libxml::~content_handler_libxml( )
{ xmlCleanupParser( ); }

//
actions::opl_parser_action& content_handler_libxml::get_action( )
{ return action_; }

//
xmlSAXHandler* content_handler_libxml::get_content_handler( )
{ return &handler_; }

//
void content_handler_libxml::set_branch_path( const boost::filesystem::path& branch_path )
{ action_.set_branch_path( branch_path ); }

} }
