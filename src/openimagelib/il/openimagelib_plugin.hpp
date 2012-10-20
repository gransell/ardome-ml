
// il - A image library representation.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef OPENIMAGELIB_PLUGIN_INC_
#define OPENIMAGELIB_PLUGIN_INC_

#ifndef BOOST_FILESYSTEM_DYN_LINK 
    #define BOOST_FILESYSTEM_DYN_LINK 
#endif
 
#include <boost/filesystem/path.hpp>
#include <boost/shared_ptr.hpp>

#include <openimagelib/il/il.hpp>

#include <openpluginlib/pl/openpluginlib.hpp>

namespace olib { namespace openimagelib { namespace il {

struct IL_DECLSPEC openimagelib_plugin : public olib::openpluginlib::openplugin
{
	virtual image_type_ptr	load(  const boost::filesystem::wpath& path ) = 0;
	virtual bool			store( const boost::filesystem::wpath& path, const image_type_ptr& image ) = 0;
};

typedef boost::shared_ptr< openimagelib_plugin > openimagelib_plugin_ptr;

} } }

#endif
