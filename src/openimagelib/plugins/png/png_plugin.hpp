
// PNG - A PNG plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef PNG_PLUGIN_INC_
#define PNG_PLUGIN_INC_

#include <boost/filesystem/path.hpp>

#include <openimagelib/il/openimagelib_plugin.hpp>

#include <openimagelib/plugins/png/config.hpp>

namespace olib { namespace openimagelib { namespace plugins { namespace PNG {

struct PNG_DECLSPEC PNG_plugin : public olib::openimagelib::il::openimagelib_plugin
{
	virtual il::image_type_ptr	load(  const boost::filesystem::path& path );
	virtual bool				store( const boost::filesystem::path& path, const il::image_type_ptr& image );
};

} } } }

#endif
