#ifndef _CORE_DESERIALIZER_H_ 
#define _CORE_DESERIALIZER_H_ 

#include "./typedefs.hpp"

namespace olib
{
   namespace opencorelib
    {
        /// Deserializes an object from a stream.
        /** Very general interface that given a stream creates an
            object, if the implementer recoginzes the bytes in 
            the stream and knows what to do with them. 
            @author Mats Lindel&ouml;f */
        class CORE_API deserializer 
        {
        public:
            deserializer();
            
            virtual ~deserializer();

            /// Creates an object from a stream, or returns an empty pointer upon failure.
            /** @param is A byte stream with a serialized object.
                    Any representation is possible, the implementor is responsible
                    to do detection.
                @return An object that inherits from opencorelib::object.
                @throws base_exception If errors occur during parsing. */
            virtual object_ptr deserialize( std::istream& is ) = 0;
        };
    }
}

#endif // _CORE_DESERIALIZER_H_ 

