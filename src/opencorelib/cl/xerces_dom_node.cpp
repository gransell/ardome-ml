
#include "precompiled_headers.hpp"
#include "./xerces_utilities.hpp"
#include "./xerces_dom_node.hpp"
#include "./str_util.hpp"

namespace olib { namespace opencorelib { namespace xml {

#if defined (XERCES_CPP_NAMESPACE)
	using namespace XERCES_CPP_NAMESPACE;
#endif

exception::exception(const std::string& msg) throw()
: msg(msg)
{
}

exception::~exception() throw()
{
}

const char* exception::what() const throw()
{
	return msg.c_str();
}


template<>
streamable_input& streamable_input::operator<<(const std::string& s) {
	onStreamInput(s);
	return *this;
}
template<>
streamable_input& streamable_input::operator<<(const std::wstring& s) {
	onStreamInput(olib::opencorelib::str_util::to_string( s ));
	return *this;
}
template<>
streamable_input& streamable_input::operator<<(const char* _s) {
	std::string s(_s);
	onStreamInput(s);
	return *this;
}
template<>
streamable_input& streamable_input::operator<<(const wchar_t* _s) {
	std::wstring s(_s);
	onStreamInput(olib::opencorelib::str_util::to_string( s ));
	return *this;
}


namespace dom {


node::attribute::attribute(attribute& attrib)
: n(attrib.n)
, key(attrib.key)
, val(attrib.val)
, changed(attrib.changed)
, exists(attrib.exists)
{
}
node::attribute::~attribute()
{
	// Flush the value, i.e. set the attribute, if it's changed

	if (changed && exists)
		n.setAttribute(key, val);
}

node::attribute::operator std::string() const
{
	return val;
}

node::attribute& node::attribute::operator=(const std::string& s)
{
	val = s;
	changed = true;
	exists = true;
	return *this;
}

node::attribute& node::attribute::operator=(const std::wstring& s)
{
	val = olib::opencorelib::str_util::to_string(s);
	changed = true;
	exists = true;
	return *this;
}

bool node::attribute::valid() const
{
	return exists;
}

node::attribute& node::attribute::remove() {
	n.removeAttribute(key);
	val = "";
	exists = false;
	changed = false;
	return *this;
}

void node::attribute::onStreamInput(const std::string& s) {
	val += s;
	changed = true;
	exists = true;
}

node::attribute::attribute(node& n, const std::string& attrib, const std::string* val)
: n(n)
, key(attrib)
, val(val ? *val : "")
, changed(false)
, exists(val)
{
}


node::node()
: n_(NULL)
, owner_(NULL)
{
}
node::node(const node& ref)
: n_(ref.n_)
, parent_(ref.parent_)
, owner_(ref.owner_)
, ns_(ref.ns_)
, node_ns_(ref.node_ns_)
, nsmap_(ref.nsmap_)
{
}
node::~node()
{
}

bool node::valid() const {
	return owner_ && n_;
}

node& node::operator=(const std::string& s) {
	clear();
	appendText(s);
	return *this;
}
node& node::operator=(const std::wstring& ws) {
	clear();
	appendText(ws);
	return *this;
}

node node::operator()(const std::string& name) {
	return createChild(name);
}

node::attribute node::operator[](const std::string& attrib) {
	std::string val;
	if (getAttribute(attrib, val)) {
		attribute a(*this, attrib, &val);
		return a;
	}
	attribute a(*this, attrib, NULL);
	return a;
}

node node::first(const std::string& s) {
	if ( !n_ )
		return node();

	DOMNodeList* children = n_->getChildNodes();

	if ( !children || !children->getLength( ) )
		return node();

	XMLSize_t len = children->getLength( );
	for ( XMLSize_t i = 0; i < len; ++i ) {
		DOMNode* np = children->item( i );

		node n(this, np);
		if ( n.getName() == s )
			return n;
	}

	return node();
}

// ns_uri is ignored, getNsUri seems not to work for some reason
node node::first(const std::string& s, const std::string& ns_uri) {
	if ( !n_ )
		return node();

	DOMNodeList* children = n_->getChildNodes();

	if ( !children || !children->getLength( ) )
		return node();

	XMLSize_t len = children->getLength( );
	for ( XMLSize_t i = 0; i < len; ++i ) {
		DOMNode* np = children->item( i );

		node n(this, np);
		if ( n.getName() == s /* && n.getNsUri() == ns_uri */ )
			return n;
	}

	return node();
}

nodes node::all(const std::string& s) {
	nodes _nodes;

	if ( !n_ )
		return _nodes;

	DOMNodeList* children = n_->getChildNodes();

	if ( !children || !children->getLength( ) )
		return _nodes;

	XMLSize_t len = children->getLength( );
	for ( XMLSize_t i = 0; i < len; ++i ) {
		DOMNode* np = children->item( i );

		node n(this, np);
		if ( n.getName() == s )
			_nodes.push_back( n );
	}

	return _nodes;
}

// ns_uri is ignored, getNsUri seems not to work for some reason
nodes node::all(const std::string& s, const std::string& ns_uri) {
	nodes _nodes;

	if ( !n_ )
		return _nodes;

	DOMNodeList* children = n_->getChildNodes();

	if ( !children || !children->getLength( ) )
		return _nodes;

	XMLSize_t len = children->getLength( );
	for ( XMLSize_t i = 0; i < len; ++i ) {
		DOMNode* np = children->item( i );

		node n(this, np);
		if ( n.getName() == s /* && n.getNsUri() == ns_uri */ )
			_nodes.push_back( n );
	}

	return _nodes;
}

std::string node::getNsUri() const {
	if ( !n_ )
		return "";
	const XMLCh* nsuri = n_->getNamespaceURI( );
	if ( !nsuri )
		return "";

	return xml::to_string(nsuri);
}

std::string node::getName() const {
	if ( !n_ )
		return "";

	const XMLCh* name = n_->getNodeName( );
	std::string s( xml::to_string( name ) );
	std::string::size_type i = s.find(':');

	return i == std::string::npos ? s : s.substr(i + 1);
}

std::string node::getValue() const {
	if ( !n_ )
		return "";
	const XMLCh* name = n_->getTextContent( );
	return xml::to_string(name);
}

node node::createChild(const std::string& _name)
{
	std::string::size_type i = _name.find(':');
	DOMNode* dn = NULL;

	std::string ns;

	if (i == 0) {
		// Explicit no namespace
		// TODO: Warning, we might need to ensure all parents use
		// explicit namespaces and the global namespace is the default
		// xml namespace
		dn = owner_->createElement(xml::from_string(_name).c_str());
	} else {
		if (i == std::string::npos) {
			// Lookup namespace from parent tags
		} else {
			// Namespaced
			ns = _name.substr(0, i);
		}

		std::string ns_uri;

		// Lookup namespace URI
		node* n = this;
		do {
			if (ns.empty())
				ns = n->node_ns_;
			if (!ns.empty()) {
				NamespaceMapIter nsi = n->nsmap_.find(ns);
				if (nsi != n->nsmap_.end()) {
					ns_uri = nsi->second;
					break;
				}
			}

			n = n->parent_;
		} while (n);
		if (!n)
			throw xml::exception("Unable to find the appropriate "
				"namespace for this tag");

		std::string name = _name;
		if (i == std::string::npos)
			name = ns + ":" + name;

		// Create namespaced element
		dn = owner_->createElementNS(
			xml::from_string(ns_uri).c_str(),
			xml::from_string(name).c_str());
	}

	n_->appendChild(dn);

	node ret(this, dn);
	ret.node_ns_ = ns;
	return ret;
}

node node::createChild(const std::string& name, const std::string& ns_uri) {
	std::string::size_type i = name.find(':');

	DOMNode* dn = NULL;
	std::string ns;

	if (i == std::string::npos) {
		// No prefixed namespace, using as default namespace
		dn = owner_->createElementNS(
			xml::from_string(ns_uri).c_str(),
			xml::from_string(name).c_str());
	} else {
		// Namespaced
		dn = owner_->createElementNS(
			xml::from_string(ns_uri).c_str(),
			xml::from_string(name).c_str());

		ns = name.substr(0, i);
	}

	n_->appendChild(dn);

	node ret(this, dn);
	ret.node_ns_.assign(ns);

	if (i == std::string::npos)
		// Set this namespace as the default namespace
		ret.ns_ = ns_uri;
	else
		// Append this namespace to this node's namespace map
		ret.nsmap_[ns] = ns_uri;

	return ret;
}

node& node::appendText(const std::string& s)
{
	xerces_string xs = xml::from_string(s);

	DOMText* dt = owner_->createTextNode(xs.c_str());

	n_->appendChild(dt);

	return *this;
}

node& node::appendText(const std::wstring& s) {
	return appendText( olib::opencorelib::str_util::to_string( s ) );
}

node& node::clear() {
	while (DOMNode* nchild = n_->getLastChild())
		n_->removeChild(nchild);
	return *this;
}

bool node::hasAttribute(const std::string& name) const
{
	DOMNamedNodeMap* dnnm = n_->getAttributes();
	return dnnm->getNamedItem(xml::from_string(name).c_str());
}

std::string node::getAttribute(const std::string& name) const
{
	std::string val;
	getAttribute(name, val);
	return val;
}

bool node::getAttribute(const std::string& name, std::string& val) const
{
	DOMNamedNodeMap* dnnm = n_->getAttributes();
	DOMNode* dn = dnnm->getNamedItem(xml::from_string(name).c_str());
	if (!dn)
		return false;
	val = to_string(dn->getNodeValue());
	return true;
}

node& node::setAttribute(const std::string& name, const std::string& val) {
	DOMNamedNodeMap* dnnm = n_->getAttributes();

	if (!dnnm)
		throw xml::exception("Could not get attribute list of node");

	DOMAttr* da = owner_->createAttribute(xml::from_string(name).c_str());

	da->setValue(xml::from_string(val).c_str());

	dnnm->setNamedItem(da);

	return *this;
}

node& node::removeAttribute(const std::string& name) {
	DOMNamedNodeMap* dnnm = n_->getAttributes();

	if (!dnnm)
		throw xml::exception("Could not get attribute list of node");

	dnnm->removeNamedItem(xml::from_string(name).c_str());

	return *this;
}

node::node(node* parent, DOMNode* nodeptr)
: n_(nodeptr)
, parent_(parent)
, owner_(nodeptr->getOwnerDocument())
, ns_()
, node_ns_()
, nsmap_()
{
}

void node::onStreamInput(const std::string& s) {
	appendText(s);
}

} } } } // namespace dom, namespace xml, namespace opencorelib, namespace olib

