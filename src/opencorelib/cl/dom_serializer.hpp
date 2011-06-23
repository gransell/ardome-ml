#ifndef _CORE_DOM_SERIALIZER_H_
#define _CORE_DOM_SERIALIZER_H_

#include "./typedefs.hpp"
#include "./serializer.hpp"
#include "./xerces_dom_node.hpp"

namespace olib
{
   namespace opencorelib
	{
		/// An interface implemented by a class that can serialize an object
		/// to an XML dom node.
		class dom_serializer : public serializer
		{
		public:
			virtual ~dom_serializer() {}

			/// Serializes an object to an XML dom node.
			/** @param os The dom node to serialize to.
				@param obj The object to serialize.
				@throws A base_exception if the object has the wrong type */
			virtual void serialize_to_dom( xml::dom::node n, const object_ptr& obj ) = 0;
		};
	}
}

#endif // _CORE_DOM_SERIALIZER_H_ 
