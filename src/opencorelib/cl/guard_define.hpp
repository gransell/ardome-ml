#ifndef _CORE_GUARD_DEFINE_H_
#define _CORE_GUARD_DEFINE_H_

#include "macro_definitions.hpp"

/// @file guard_define.h
/** Defines a macro, ARGUARD that is used to execute arbitrary function
	at scope exit. Example of usage:
	<pre>
		ARGUARD(boost::bind<void>(::OutputDebugString, L"Exit Long_task 1\n"))
	</pre> 
	This will execute the platform specific (windows) function OutputDebugString when ARGUARD goes
	out of scope.*/

/**	@def ARGUARD
	@param func Probably a boost::function object created with boost::bind.*/
#define ARGUARD(func) boost::shared_ptr<void> ARMAKE_UNIQUE_NAME(arguard_) (static_cast<void*>(0), func)


#endif // _CORE_GUARD_DEFINE_H_

