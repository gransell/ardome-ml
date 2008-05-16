
// 3D_lightmap - A lightmap generator plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef LIGHTMAP3D_PLUGIN_INC_
#define LIGHTMAP3D_PLUGIN_INC_

#include <openimagelib/plugins/3D_lightmap/config.hpp>
#include <openimagelib/il/openimagelib_plugin.hpp>

namespace olib { namespace openimagelib { namespace plugins { namespace lightmap {

class LIGHTMAP3D_DECLSPEC lightmap3D_plugin : public olib::openimagelib::il::openimagelib_plugin
{
public:
	virtual il::image_type_ptr	load(  const boost::filesystem::path& path );
	virtual bool				store( const boost::filesystem::path& path, const il::image_type_ptr& image );
};

} } } }

#endif
