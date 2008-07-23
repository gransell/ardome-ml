/* -*- tab-width: 4; indent-tabs-mode: t -*- */ 
// ml - A media library representation.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef OPENMEDIALIB_PLUGIN_INC_
#define OPENMEDIALIB_PLUGIN_INC_
#ifndef BOOST_FILESYSTEM_DYN_LINK 
    #define BOOST_FILESYSTEM_DYN_LINK 
#endif


#include <openmedialib/ml/ml.hpp>

#include <openpluginlib/pl/openpluginlib.hpp>
#include <boost/shared_ptr.hpp>
#include <openpluginlib/pl/utf8_utils.hpp>
#include <openpluginlib/pl/log.hpp>

#include <boost/filesystem/path.hpp>

#ifdef _MSC_VER
#	pragma warning ( push )
#	pragma warning ( disable: 4100 )
#endif

namespace olib { namespace openmedialib { namespace ml {

class ML_DECLSPEC openmedialib_plugin : public olib::openpluginlib::openplugin
{
protected:
	explicit openmedialib_plugin( );
	virtual ~openmedialib_plugin( );

public:
	virtual input_type_ptr load(  const boost::filesystem::path &path ) { return input( olib::openpluginlib::to_wstring( path.external_file_string( ).c_str( ) ) ); }
	virtual input_type_ptr input(  const openpluginlib::wstring& /*resource*/ ) { return input_type_ptr( ); }
	virtual store_type_ptr store( const openpluginlib::wstring& /*resource*/, const frame_type_ptr& /*frame*/ ) { return store_type_ptr( ); }
	virtual filter_type_ptr filter( const openpluginlib::wstring & ) { return filter_type_ptr( ); }
};

	typedef boost::shared_ptr< openmedialib_plugin > openmedialib_plugin_ptr;
} } }

#ifdef _MSC_VER
#	pragma warning ( pop )
#endif

#endif
