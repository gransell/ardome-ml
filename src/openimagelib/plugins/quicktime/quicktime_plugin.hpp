
// QuickTime - An QuickTime plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef QUICKTIME_PLUGIN_INC_
#define QUICKTIME_PLUGIN_INC_

#include <boost/filesystem/path.hpp>

#include <openimagelib/plugins/quicktime/config.hpp>
#include <openimagelib/il/openimagelib_plugin.hpp>

namespace olib { namespace openimagelib { namespace plugins { namespace QT {

struct QUICKTIME_DECLSPEC QT_plugin : public olib::openimagelib::il::openimagelib_plugin
{
	virtual il::image_type_ptr	load(  const boost::filesystem::path& path );
	virtual bool				store( const boost::filesystem::path& path, const il::image_type_ptr& image );
};

} } } }

#endif
