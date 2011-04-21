
#include "precompiled_headers.hpp"
#include "./xerces_utilities.hpp"
#include "./xerces_dom_document.hpp"

namespace olib { namespace opencorelib { namespace xml {

#if defined (XERCES_CPP_NAMESPACE)
	using namespace XERCES_CPP_NAMESPACE;
#endif


namespace dom {

document::document()
{
	DOMImplementation* di = DOMImplementation::getImplementation();

	doc = DocPtr(di->createDocument());

	// Set these in the node superclass
	n_ = &*doc;
	parent_ = NULL;
	owner_ = &*doc;
}

document::document(const char* qn, const char* ns)
{
	DOMImplementation* di = DOMImplementation::getImplementation();

	doc = DocPtr(di->createDocument(
		to_x_string(ns).c_str(),
		to_x_string(qn).c_str(),
		NULL));

	// Set these in the node superclass
	n_ = &*doc;
	parent_ = NULL;
	owner_ = &*doc;
}

document::document(document& ref)
: node(ref)
, doc(ref.doc)
{
}

document::~document()
{
}

document::operator XERCES_CPP_NAMESPACE::DOMDocument&()
{
	return *doc;
}
document::operator const XERCES_CPP_NAMESPACE::DOMDocument&() const
{
	return *doc;
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
	if (!doc)
		return false;

	DOMImplementation* di = DOMImplementation::getImplementation();

	DOMWriter* dw = di->createDOMWriter();

	if (!dw)
		return false;

	DOMWriterFilter* dwf = dw->getFilter();

	if (!dwf)
		return false;

	// Choose to (or not to) print the xml header (<?xml ... ?>)
	unsigned long flt = dwf->getWhatToShow();
	flt &= ~DOMNodeFilter::SHOW_ENTITY;
	if (!asFragment)
		flt |= DOMNodeFilter::SHOW_ENTITY;
	dwf->setWhatToShow(flt);

	dw->setFeature(to_x_string("format-pretty-print").c_str(), true);
	dw->setEncoding(to_x_string("utf-8").c_str());
	dw->setNewLine(to_x_string("\n").c_str());
	error_handler e;
	dw->setErrorHandler(&e);
	bool ret = dw->writeNode(ft, *doc);

	delete dw;
	return ret;
}

} // namespace dom

} } } // namespace xml, namespace opencorelib, namespace olib

