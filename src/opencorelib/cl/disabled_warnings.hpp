#ifndef _CORE_DISABLEDWARNINGS_H_
#define _CORE_DISABLEDWARNINGS_H_

#include "./platform.hpp"

// The following macros could be used to silence the complier about unused variables

#ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
	// The following is a warning about missing dll-interface for template members.
	// Since the consumers of the dll's are other C++ dll's this is no
	// problem: the template will be reinstanciated if the template is
	// used in another assembly.
	#pragma warning( disable: 4251 ) 

	// Disables warnings about boost::noncopyable not being exported as a dll-class.
	// non dll-interface class 'boost::noncopyable' 
	#pragma warning( disable: 4275 ) 


	// decorated name length exceeded, name was truncated
	#pragma warning(disable:4503) 

#endif

#endif // _CORE_DISABLEDWARNINGS_H_

