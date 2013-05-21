
#ifndef OPENCORELIB_XERCES_DOM_DOCUMENT_H_
#define OPENCORELIB_XERCES_DOM_DOCUMENT_H_

#include "./typedefs.hpp"

#include "./xerces_dom_node.hpp"

namespace olib { namespace opencorelib { namespace xml {

struct DocPtrNullDeleter {
	void operator()(XERCES_CPP_NAMESPACE::DOMDocument* docptr) {}
};

typedef boost::shared_ptr<XERCES_CPP_NAMESPACE::DOMDocument> DocPtr;
typedef boost::shared_ptr<XERCES_CPP_NAMESPACE::XercesDOMParser> DomParserPtr;

namespace dom {

class fragment_filter;
class document;
class fragment;

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

protected:
	bool writeNode(XERCES_CPP_NAMESPACE::XMLFormatTarget* ft,
	               bool asFragment = false) const;

	DocPtr doc;
};

/*
 * Very simple and basic DOM parser.
 * NOTE: All referenced document and node pointers from this fragment is owned
 * by the fragment instance. When the instance is deleted, the node pointers
 * are invalid.
 */
class CORE_API fragment : public document {
public:
	fragment(const std::string& xmltext);

private:
	DomParserPtr parser_;
};

/*
 * Simple DOM parser.
 * NOTE: The same rules applies to this as to the fragment parser
 */
class CORE_API file_document : public document {
public:
	file_document(const std::string& filename);
	file_document(std::istream &xml_input_stream);
};

} // namespace dom

} } } // namespace xml, namespace opencorelib, namespace olib

#endif // OPENCORELIB_XERCES_DOM_DOCUMENT_H_

