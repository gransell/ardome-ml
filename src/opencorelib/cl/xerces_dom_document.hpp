
#ifndef OPENCORELIB_XERCES_DOM_NODE_H_
#define OPENCORELIB_XERCES_DOM_NODE_H_

#include "./typedefs.hpp"
#include "./xml_sax_parser.hpp"

namespace olib { namespace opencorelib { namespace xml {

namespace dom {

class document : public node {
public:
	// Creates a completely empty document without root element
	document();

	// Creates a document with root element in namespace ns with qualified
	// name qn.
	document(const char* qn, const char* ns = NULL);

	// Creates a new document object pointing to THE SAME DOMDocument as
	// in object 'ref'
	document(document& ref);

	~document();

	// Cloaking operators to xerces DOMDocument
	operator XERCES_CPP_NAMESPACE::DOMDocument&();
	operator const XERCES_CPP_NAMESPACE::DOMDocument&() const;

	bool serializeIntoString(std::string& s) const;
	bool serializeToFile(std::string& filename) const;

private:
	document(const document&) {}

	bool writeNode(XMLFormatTarget* ft) const;

	XERCES_CPP_NAMESPACE::DOMDocument* doc;
};

} // namespace dom

} } } // namespace xml, namespace opencorelib, namespace olib

#endif // OPENCORELIB_XERCES_DOM_NODE_H_

