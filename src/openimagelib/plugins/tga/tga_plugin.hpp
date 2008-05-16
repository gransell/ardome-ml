
// TGA - A TGA plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef TGA_PLUGIN_INC_
#define TGA_PLUGIN_INC_

#include <boost/filesystem/path.hpp>

#include <openimagelib/plugins/tga/config.hpp>
#include <openimagelib/il/openimagelib_plugin.hpp>

namespace olib { namespace openimagelib { namespace plugins { namespace TGA {

class TGA_DECLSPEC TGA_plugin : public olib::openimagelib::il::openimagelib_plugin
{
public:
	virtual il::image_type_ptr	load(  const boost::filesystem::path& path );
	virtual bool				store( const boost::filesystem::path& path, const il::image_type_ptr& image );
};

} } } }

#endif
