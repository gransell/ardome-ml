
// HDR - A HDR plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef HDR_PLUGIN_INC_
#define HDR_PLUGIN_INC_

#include <boost/filesystem/path.hpp>

#include <openimagelib/plugins/hdr/config.hpp>
#include <openimagelib/il/openimagelib_plugin.hpp>

namespace olib { namespace openimagelib { namespace plugins { namespace HDR {

struct HDR_DECLSPEC HDR_plugin : public olib::openimagelib::il::openimagelib_plugin
{
	virtual il::image_type_ptr	load(  const boost::filesystem::path& path );
	virtual bool				store( const boost::filesystem::path& path, const il::image_type_ptr& image );
};

} } } }

#endif
