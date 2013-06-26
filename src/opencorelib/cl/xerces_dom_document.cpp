
#include "precompiled_headers.hpp"
#include "./xerces_utilities.hpp"
#include "./xerces_dom_document.hpp"

namespace olib { namespace opencorelib { namespace xml {

#if defined (XERCES_CPP_NAMESPACE)
	using namespace XERCES_CPP_NAMESPACE;
#endif

namespace
{
	//Error handler for the DOM parser
	class dom_parser_error_handler : public ErrorHandler
	{
		virtual void warning( const SAXParseException &exc )
		{
			ARLOG_WARN( "XML DOM parsing warning at line %1%: %2%" )
				( exc.getLineNumber() )
				( exc.getMessage() );
		}

		virtual void error( const SAXParseException &exc )
		{
			throw exc;
		}

		virtual void fatalError( const SAXParseException &exc )
		{
			throw exc;
		}

		virtual void resetErrors()
		{
		}
	};
}

namespace dom {

fragment_filter::fragment_filter()
: what_to_show_(~DOMNodeFilter::SHOW_ENTITY)
{
}

fragment_filter::~fragment_filter()
{
}

short fragment_filter::acceptNode(const DOMNode* node) const
{
	return node && ( node->getNodeType() & what_to_show_ );
}

unsigned long fragment_filter::getWhatToShow() const
{
	return what_to_show_;
}

void fragment_filter::setWhatToShow(unsigned long toShow)
{
	what_to_show_ = toShow;
}


document::document()
{
	DOMImplementation* di = DOMImplementation::getImplementation();

	doc_ = DocPtr(di->createDocument());

	// Set these in the node superclass
	n_ = doc_.get();
	parent_ = NULL;
	owner_ = doc_.get();
}

document::document(const char* qn, const char* ns)
{
	DOMImplementation* di = DOMImplementation::getImplementation();

	doc_ = DocPtr(di->createDocument(
		to_x_string(ns).c_str(),
		to_x_string(qn).c_str(),
		NULL));

	// Set these in the node superclass
	n_ = doc_.get();
	parent_ = NULL;
	owner_ = doc_.get();
}

document::document(document& ref)
: node(ref)
, doc_(ref.doc_)
{
}

document::~document()
{
}

document::operator XERCES_CPP_NAMESPACE::DOMDocument&()
{
	return *doc_;
}
document::operator const XERCES_CPP_NAMESPACE::DOMDocument&() const
{
	return *doc_;
}

bool document::serializeIntoString(std::string& s, bool asFragment) const
{
	MemBufFormatTarget t(1024 * 64);

	bool ret = writeNode(&t, asFragment);

	if (ret)
		s.assign((const char*)t.getRawBuffer(), t.getLen());
	return ret;
}

bool document::serializeToFile(std::string& s, bool asFragment) const
{
	LocalFileFormatTarget t(s.c_str());

	return writeNode(&t, asFragment);
}

bool document::writeNode(XMLFormatTarget* ft, bool asFragment) const
{
	if (!doc_)
		return false;

	DOMImplementation* di = DOMImplementation::getImplementation();

	DOMWriter* dw = di->createDOMWriter();

	if (!dw)
		return false;

	fragment_filter ff;
	dw->setFilter(asFragment ? &ff : NULL);

	dw->setFeature(to_x_string("format-pretty-print").c_str(), true);
	dw->setEncoding(to_x_string("utf-8").c_str());
	dw->setNewLine(to_x_string("\n").c_str());
	error_handler e;
	dw->setErrorHandler(&e);
	bool ret = dw->writeNode(ft, *doc_);

	delete dw;
	return ret;
}


fragment::fragment(const std::string& xmltext)
{
	XercesDOMParser parser;
	MemBufInputSource membuf((const XMLByte*)xmltext.c_str(), xmltext.size(), (const XMLCh*)NULL);

	dom_parser_error_handler e;
	parser.setErrorHandler(&e);
	parser.parse(membuf);

	doc_ = DocPtr(parser.adoptDocument());
	n_ = doc_.get();
}


file_document::file_document(const std::string& filename)
{
	XercesDOMParser parser;

	xerces_string x_filename = xml::from_string(filename);
	LocalFileInputSource filebuf(x_filename.c_str());

	dom_parser_error_handler e;
	parser.setErrorHandler(&e);
	parser.parse(filebuf);

	doc_ = DocPtr(parser.adoptDocument());
	n_ = doc_.get();
}

file_document::file_document(std::istream &xml_input_stream)
{
	XercesDOMParser parser;

	olib::opencorelib::std_input_source stream_buf(xml_input_stream);

	dom_parser_error_handler e;
	parser.setErrorHandler(&e);
	parser.parse(stream_buf);

	doc_ = DocPtr(parser.adoptDocument());
	n_ = doc_.get();
}


} // namespace dom

} } } // namespace xml, namespace opencorelib, namespace olib

