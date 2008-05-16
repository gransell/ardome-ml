
// TIFF - A TIFF plugin to il.

// Copyright (C) 2005-2006 Visual Media FX Ltd.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef TIFF_PLUGIN_INC_
#define TIFF_PLUGIN_INC_

#include <boost/filesystem/path.hpp>

#include <openimagelib/plugins/tiff/config.hpp>
#include <openimagelib/il/openimagelib_plugin.hpp>

namespace olib { namespace openimagelib { namespace plugins { namespace TIFF {

class TIFF_DECLSPEC TIFF_plugin : public olib::openimagelib::il::openimagelib_plugin
{
public:
	virtual il::image_type_ptr	load(  const boost::filesystem::path& path );
	virtual bool				store( const boost::filesystem::path& path, const il::image_type_ptr& image );
};

} } } }

#endif
