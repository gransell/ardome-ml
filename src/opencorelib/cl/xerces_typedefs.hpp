#ifndef _CORE_XERCES_TYPEDEFS_H_
#define _CORE_XERCES_TYPEDEFS_H_

#include <xercesc/util/XercesDefs.hpp>

namespace olib { namespace opencorelib {

// The size type taken by functions differs between version 2 and 3
#if XERCES_VERSION_MAJOR >= 3
typedef XMLSize_t xerces_size_type;
typedef XMLFilePos xerces_file_pos;
#else
typedef unsigned int xerces_size_type;
typedef unsigned int xerces_file_pos;
#endif

// The type of string that we get out of the sax traverser in the callbacks
typedef std::basic_string<XMLCh> xerces_string;
	
} /* opencorelib */ } /* olib */


#endif // _CORE_XERCES_TYPEDEFS_H_

