
#include "precompiled_headers.hpp"
#include "xerces_dom_builder.hpp"
#include "./xerces_utilities.hpp"

namespace olib { namespace opencorelib { namespace xml {

#if defined (XERCES_CPP_NAMESPACE) && XERCES_CPP_NAMESPACE
	using namespace XERCES_CPP_NAMESPACE;
#endif

namespace dom {

node::attribute::attribute(attribute& attrib)
: n(attrib.n)
, key(attrib.key)
, val(attrib.val)
, changed(false)
{
}
node::attribute::~attribute()
{
	// Flush the value, i.e. set the attribute, if it's changed

	if (changed)
		n.setAttribute(key, val);
}

node::attribute::operator std::string() const {
	return val;
}

node::attribute& node::attribute::operator=(const std::string& val s) {
	val = s;
	changed = true;
}

node::attribute::attribute(node& n, const std::string& attrib, const std::string& val)
: n(n)
, key(attrib)
, val(val)
, changed(false)
{
}


node::node()
: owner(NULL)
{
}
node::~node()
{
}

node::attribute node::operator[](const std::string& attrib) {
	;
}

node node::createChild(const std::string& name)
{
	DOMText* dt = owner->createTextNode(xs.c_str());

	n->appendChild(dt);
}

node& node::appendText(const std::string& s)
{
	xerces_string xs = to_x_string(s);

	DOMText* dt = owner->createTextNode(xs.c_str());

	n->appendChild(dt);
}

node& node::setAttribute(const std::string& name, const std::string& val) {
	createAttribute 	( 	const XMLCh *  	name 	 )  	[
	;
}

node::node(XERCES_CPP_NAMESPACE::DOMNode* nodeptr)
: n(nodeptr)
, owner(nodeptr->getOwnerDocument())
{
}

void node::flush() {
	// Apply the string in the internal strinbuf into the xml node

	// Flush ostream buffer
	this->std::ostringstream::flush();
	// Get the string content of the ostream
	std::string s = this->std::ostringstream::str();

	if (!s.empty()) {
		// Append text to xml
		n.appendText(s);
		// Clear ostream buffer
		this->std::ostringstream::str("");
	}
}

} } } // namespace xml, namespace opencorelib, namespace olib

