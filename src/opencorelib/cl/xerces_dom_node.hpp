
#ifndef OPENCORELIB_XERCES_DOM_NODE_H_
#define OPENCORELIB_XERCES_DOM_NODE_H_

#include "./typedefs.hpp"
#include <sstream>

#include "./xerces_headers.hpp"

namespace olib { namespace opencorelib { namespace xml {

class exception : public std::exception {
public:
	exception(const std::string& msg) throw();
	virtual ~exception() throw();

	virtual const char* what() const throw();

private:
	std::string msg;
};

class streamable_input {
public:
	template<typename T>
	streamable_input& operator<<(const T& t) {
		std::ostringstream ss;
		ss << t;
		ss.flush();
		onStreamInput(ss.str());
		return *this;
	}

	virtual void onStreamInput(const std::string& s) = 0;
};

template<> streamable_input& streamable_input::operator<<(const std::string& s);
template<> streamable_input& streamable_input::operator<<(const std::wstring& s);

namespace dom {

class node;
class document;

class node : public streamable_input {
public:
	class attribute : public streamable_input {
	public:
		attribute(attribute& attrib);
		~attribute();

		operator std::string() const;
		attribute& operator=(const std::string& val);
		attribute& operator=(const std::wstring& val);

		operator bool();

		attribute& remove();

	protected:
		void onStreamInput(const std::string& s);

	private:
		friend class node;
		attribute(node& n, const std::string& attrib, const std::string* val);

		node& n;
		std::string key, val;
		bool changed;
		bool exists;
	};

	node();
	node(const node& ref);
	virtual ~node();

	node& operator=(const std::string& s); // Sets value
	node& operator=(const std::wstring& ws); // Sets value

	node operator()(const std::string& name); // Alias to createChild()
	attribute operator[](const std::string& attrib);

	/*
	 * Creates a child node.
	 * If name begins with ':', it will be created without local namespace
	 * prefix, hence belong to the default namespace within its context.
	 * If the name begins with "[something]:", that something prefix will
	 * be looked up, and that namespace will be used, unless ns_uri is
	 * specified, whereby that ns URI will be used.
	 * If the name doesn't contain ':', the parent's namespace and prefix
	 * will be inherited.
	 */
	node createChild(const std::string& name);
	node createChild(const std::string& name, const std::string& ns_uri);
	node& appendText(const std::string& s);
	node& appendText(const std::wstring& s);

	node& clear(); // Removes all children

	bool hasAttribute(const std::string& name) const;
	std::string getAttribute(const std::string& name) const;
	bool getAttribute(const std::string& name, std::string& val) const;
	node& setAttribute(const std::string& name, const std::string& val);
	node& removeAttribute(const std::string& name);

protected:
	node(node* parent, XERCES_CPP_NAMESPACE::DOMNode* nodeptr);

	void onStreamInput(const std::string& s);

	// Flushes the ostringstream and appends the content as a text node
	void flush();

	XERCES_CPP_NAMESPACE::DOMNode* n_;
	node* parent_;

private:
	friend class document;
	XERCES_CPP_NAMESPACE::DOMDocument* owner_;

	// The default namespace
	std::string ns_;
	std::string node_ns_; // The ns prefix of this node

	// A namespace map for this particular node
	typedef std::map< std::string, std::string > NamespaceMap;
	typedef NamespaceMap::iterator NamespaceMapIter;
	NamespaceMap nsmap_;
};

} // namespace dom

} } } // namespace xml, namespace opencorelib, namespace olib

#endif // OPENCORELIB_XERCES_DOM_NODE_H_

