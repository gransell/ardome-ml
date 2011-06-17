#ifndef _CORE_SERIALIZER_H_ 
#define _CORE_SERIALIZER_H_ 

#include "./typedefs.hpp"

namespace olib
{
   namespace opencorelib
    {
        /// An interface implemented by a class that can serialize an object to a byte stream.
        /** @author Mats Lindel&ouml;f */
        class serializer 
        {
        public:
            virtual ~serializer() {}

            /// Serializes an object to a byte stream.
            /** @param os The byte stream to write to.
                @param obj The object to serialize.
                @throws A base_exception if the object has the wrong type */
            virtual void serialize( std::ostream& os, const object_ptr& obj ) = 0;
        };
    }
}

#endif // _CORE_SERIALIZER_H_ 

