
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005 Visual Media FX Ltd.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef CONTENT_HANDLER_LIBXML_INC_
#define CONTENT_HANDLER_LIBXML_INC_
#ifndef BOOST_FILESYSTEM_DYN_LINK 
    #define BOOST_FILESYSTEM_DYN_LINK 
#endif


#include <boost/filesystem/path.hpp>

#include <openpluginlib/pl/opl_parser_action.hpp>

namespace olib { namespace openpluginlib {

class content_handler_libxml
{
public:
	explicit content_handler_libxml( );
	~content_handler_libxml( );

	actions::opl_parser_action&					get_action( );
	xmlSAXHandler*								get_content_handler( );
	void										set_branch_path( const boost::filesystem::path& branch_path );
				
private:
	actions::opl_parser_action 	action_;
	xmlSAXHandler				handler_;
};

} }

#endif
