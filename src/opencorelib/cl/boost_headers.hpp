#ifndef _CORE_BOOST_HEADERS_H_
#define _CORE_BOOST_HEADERS_H_

#include "./platform.hpp"
#include "./boost_link_defines.hpp"

// Must be placed before any other includes...
#ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
    #pragma warning(push)
    #pragma warning(disable:4512)
    #pragma warning(disable:4100)

#include <boost/variant.hpp>

#endif // OLIB_COMPILED_WITH_VISUAL_STUDIO

#ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO  
    #pragma warning(pop)
#endif

// Boost headers
#include <boost/functional.hpp>
#include <boost/function.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_array.hpp>
#include <boost/ref.hpp>


#ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO 
    #pragma warning(push)
    // We need to disable these if we are compiling at warning level 4
    #pragma warning(disable:4511) // copy constructor could not be generated
    #pragma warning(disable:4512) // assignment operator could not be generated
    #pragma warning(disable:4127) // conditional expression is constant
    #pragma warning(disable:4701)   // local variable 'result' may be used without having been initialized.
#endif
    
    #include <boost/lexical_cast.hpp> // Conversion routines

#ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO 
    #pragma warning(pop)
#endif

#include <boost/thread.hpp>

#ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
    #pragma warning (push)
    #pragma warning (disable:4512)
#endif
	#include <boost/assign/list_of.hpp>
	#include <boost/assign/list_inserter.hpp>
	#include <boost/assign/std/vector.hpp>

#ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
    #pragma warning (pop)
#endif

#ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
    #pragma warning( push )
    #pragma warning( disable: 4511 )
    #pragma warning( disable: 4512 )
    #pragma warning( disable: 4267 )
    #pragma warning( disable: 4244 )
	#pragma warning( disable: 4996 ) // 'function': was declared deprecated
#endif
	#include <boost/format.hpp>
#ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
#pragma warning (pop)
#endif

#ifndef BOOST_SIGNALS_DYN_LINK 
    #define BOOST_SIGNALS_DYN_LINK 
#endif
#ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
    #pragma warning( push )
    #pragma warning( disable: 4251 )
    #pragma warning( disable: 4275 )
    #pragma warning( disable: 4511 )
    #pragma warning( disable: 4512 )
    #pragma warning( disable: 4675 )
#endif
	#include <boost/signal.hpp>
#ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
#pragma warning (pop)
#endif

#include <boost/scoped_array.hpp>

#endif // _CORE_BOOST_HEADERS_H_

