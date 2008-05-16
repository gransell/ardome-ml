
// JPG - A JPG plugin to il.

// Copyright (C) 2005 Visual Media FX Ltd.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef JPG_PLUGIN_INC_
#define JPG_PLUGIN_INC_

#include <boost/filesystem/path.hpp>

#include <openimagelib/plugins/jpg/config.hpp>
#include <openimagelib/il/openimagelib_plugin.hpp>

namespace olib { namespace openimagelib { namespace plugins { namespace JPG {

class JPG_DECLSPEC JPG_plugin : public olib::openimagelib::il::openimagelib_plugin
{
public:
	typedef il::image<unsigned char, il::surface_format> image_type;
	typedef boost::shared_ptr<image_type>				 image_type_ptr;

public:
	virtual image_type_ptr	load(  const boost::filesystem::path& path );
	virtual bool			store( const boost::filesystem::path& path, const image_type_ptr& image );
};

} } } }

#endif
