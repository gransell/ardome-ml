
#include "precompiled_headers.hpp"
#include "./xerces_utilities.hpp"
#include "./xerces_sax_traverser.hpp"

namespace olib { namespace opencorelib { namespace xml {

#if defined (XERCES_CPP_NAMESPACE) && XERCES_CPP_NAMESPACE
	using namespace XERCES_CPP_NAMESPACE;
#endif

CORE_API xerces_string to_x_string(const char* str) {
	return to_x_string(str, strlen(str));
}

error_handler::error_handler() {
}
error_handler::~error_handler() {
}

bool error_handler::handleError(const DOMError& e) {
	ARLOG_ERR("XML Error (of severity %i): %s")
		(e.getSeverity())
		(x_to_t_string(e.getMessage()));
	return true; // Continue anyway
}

} } } // namespace xml, namespace opencorelib, namespace olib

