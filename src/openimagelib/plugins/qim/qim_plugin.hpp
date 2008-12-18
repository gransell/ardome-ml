
// QIM - A QImage plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef QIM_PLUGIN_INC_
#define QIM_PLUGIN_INC_

#include <boost/filesystem/path.hpp>

#include <openimagelib/il/openimagelib_plugin.hpp>

#include <openimagelib/plugins/qim/config.hpp>

namespace olib { namespace openimagelib { namespace plugins { namespace QIM {

struct QIM_DECLSPEC QIM_plugin : public olib::openimagelib::il::openimagelib_plugin
{
	virtual il::image_type_ptr	load(  const boost::filesystem::path& path );
	virtual bool				store( const boost::filesystem::path& path, const il::image_type_ptr& image );
};

} } } }

#endif
