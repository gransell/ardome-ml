
#ifndef OPENCORELIB_XERCES_DOM_DOCUMENT_H_
#define OPENCORELIB_XERCES_DOM_DOCUMENT_H_

#include "./typedefs.hpp"

#include "./xerces_dom_node.hpp"

namespace olib { namespace opencorelib { namespace xml {

typedef boost::shared_ptr<XERCES_CPP_NAMESPACE::DOMDocument> DocPtr;

namespace dom {

class fragment_filter : public XERCES_CPP_NAMESPACE::DOMWriterFilter {
public:
	fragment_filter();
	virtual ~fragment_filter();

protected:
	virtual short acceptNode(const XERCES_CPP_NAMESPACE::DOMNode* node) const;
	virtual unsigned long getWhatToShow() const;
	virtual void setWhatToShow(unsigned long toShow);

private:
	unsigned long what_to_show_;
};

class CORE_API document : public node {
public:
	// Creates a completely empty document without root element
	document();

	// Creates a document with root element in namespace ns with qualified
	// name qn.
	document(const char* qn, const char* ns = NULL);

	// Creates a new document object pointing to THE SAME DOMDocument as
	// in object 'ref'
	document(document& ref);

	virtual ~document();

	// Cloaking operators to xerces DOMDocument
	operator XERCES_CPP_NAMESPACE::DOMDocument&();
	operator const XERCES_CPP_NAMESPACE::DOMDocument&() const;

	bool serializeIntoString(std::string& s,
	                         bool asFragment = false) const;
	bool serializeToFile(std::string& filename,
	                     bool asFragment = false) const;

private:
	document(const document&) {}

	bool writeNode(XERCES_CPP_NAMESPACE::XMLFormatTarget* ft,
	               bool asFragment = false) const;

	DocPtr doc;
};

} // namespace dom

} } } // namespace xml, namespace opencorelib, namespace olib

#endif // OPENCORELIB_XERCES_DOM_DOCUMENT_H_

