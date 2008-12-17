#ifndef _CORE_OBJECT_H_
#define _CORE_OBJECT_H_

#include "./typedefs.hpp"

namespace olib
{
	namespace opencorelib
	{
        /// Base class for objects that want to be able to stand in as generic objects.
        /** This is useful in for instance a plug-in architecture
            where we want to be able to create generic objects.
            @see plugin_class_base_implementation   
            <bindgen>
                <attribute name="ref-counted" value="yes"></attribute>
            </bindgen>
            */
		class CORE_API object
		{		
		public: 
            /// Virtual destructor to ensure correct destruction in derived classes.
			virtual ~object();
		};
	}
}

#endif // _CORE_OBJECT_H_

