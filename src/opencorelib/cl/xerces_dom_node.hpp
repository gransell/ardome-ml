
#ifndef OPENCORELIB_XERCES_DOM_NODE_H_
#define OPENCORELIB_XERCES_DOM_NODE_H_

#include "./typedefs.hpp"
#include <sstream>

namespace olib { namespace opencorelib { namespace xml {

boost::shared_ptr<XERCES_CPP_NAMESPACE::DOMNode> NodePtr;

namespace dom {

class node : public std::ostringstream {
public:
	class attribute {
	public:
		attribute(attribute& attrib);
		~attribute();

		operator std::string() const;
		attribute& operator=(const std::string& val);

	private:
		attribute(node& n, const std::string& attrib, const std::string& val);

		node& n;
		std::string key, val;
		bool changed;
	};

	node();
	~node();

	attribute operator[](const std::string& attrib);

	node createChild(const std::string& name);
	node& appendText(const std::string& s);
	node& setAttribute(const std::string& name, const std::string& val);

protected:
	node(XERCES_CPP_NAMESPACE::DOMNode* nodeptr);

	// Flushes the ostringstream and appends the content as a text node
	void flush();

	NodePtr n;

private:
	DOMDocument owner;
};

} // namespace dom

} } } // namespace xml, namespace opencorelib, namespace olib

#endif // OPENCORELIB_XERCES_DOM_NODE_H_

