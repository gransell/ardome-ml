
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#define _WIN32_DCOM
#include <windows.h>
#include <objbase.h>
#endif

#include <openpluginlib/pl/opl_importer.hpp>
#include <openpluginlib/pl/log.hpp>

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>
#include <xercesc/framework/LocalFileInputSource.hpp>
#include "../../opencorelib/cl/xerces_sax_traverser.hpp"

XERCES_CPP_NAMESPACE_USE

namespace olib { namespace openpluginlib {

opl_importer::opl_importer( )
: auto_load( false )
{
}

opl_importer::~opl_importer( )
{
}

void opl_importer::operator( )( const boost::filesystem::path& file )
{
	namespace fs = boost::filesystem;

	XMLPlatformUtils::Initialize();

	action_.set_opl_path( file );

	boost::scoped_ptr<SAX2XMLReader> parser( XMLReaderFactory::createXMLReader() );

	parser->setContentHandler(this);
	parser->setErrorHandler(this);

	parser->parse( opencorelib::xml::from_string( file.native( ) ).c_str( ) );

	plugins = action_.plugins;
	auto_load = action_.get_auto_load();

	XMLPlatformUtils::Terminate();
}

void opl_importer::startElement (
    const XMLCh* const uri,
    const XMLCh* const localname,
    const XMLCh* const qname,
    const Attributes& attrs )
{
	action_.set_attrs(&attrs);
	action_.dispatch( opencorelib::xml::to_wstring( localname ) );
}

void opl_importer::error(const SAXParseException& exc)
{
	PL_LOG(level::error, opencorelib::xml::to_string( exc.getMessage() ) );
}

} }
