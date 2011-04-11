
#ifndef OPENCORELIB_XERCES_UTILITIES_H_
#define OPENCORELIB_XERCES_UTILITIES_H_

#include "./typedefs.hpp"
#include "./core_enums.hpp"

#include "./xerces_headers.hpp"

#include "./xerces_sax_traverser.hpp" // TODO: Remove this include and move code here instead


/** @file xerces_utilities.h
    amf wrapper class around the xerces API. */

namespace olib { namespace opencorelib { namespace xml {


/// Convert a char array to xml string
/** Uses str_util::to_wstring */
CORE_API xerces_string to_x_string(const char* str);


class error_handler : public XERCES_CPP_NAMESPACE::DOMErrorHandler {
public:
	error_handler();
	~error_handler();

	bool handleError(const XERCES_CPP_NAMESPACE::DOMError& domError);
};


} } } // namespace xml, namespace opencorelib, namespace olib

#endif // OPENCORELIB_XERCES_UTILITIES_H_

