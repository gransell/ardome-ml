#ifndef SERIALIZATION_HELPERS_H_INCLUDED_
#define SERIALIZATION_HELPERS_H_INCLUDED_

#include <opencorelib/cl/core.hpp>
#include <opencorelib/cl/minimal_string_defines.hpp>
#include <opencorelib/cl/object.hpp>

#include <istream>
#include <ostream>

namespace olib{ namespace opencorelib{

	namespace serialization_mode
	{
		enum type{ text, binary, base64 };
	}

	CORE_API void serialize_to_stream( object_ptr obj, std::ostream &stream, const olib::t_string &serializer_plugin, serialization_mode::type mode = serialization_mode::text );

	CORE_API olib::t_string serialize_to_string( object_ptr obj, const olib::t_string &serializer_plugin, serialization_mode::type mode = serialization_mode::text );

	CORE_API void serialize_to_path( object_ptr obj, const olib::t_path &filepath, const olib::t_string &serializer_plugin, serialization_mode::type mode = serialization_mode::text );

	CORE_API object_ptr deserialize_from_stream( std::istream &stream, const olib::t_string &deserializer_plugin, serialization_mode::type mode = serialization_mode::text );

	CORE_API object_ptr deserialize_from_string( const olib::t_string &str, const olib::t_string &deserializer_plugin, serialization_mode::type mode = serialization_mode::text );

	CORE_API object_ptr deserialize_from_path( const olib::t_path &filepath, const olib::t_string &deserializer_plugin, serialization_mode::type mode = serialization_mode::text );
	
} }

#endif //#ifndef SERIALIZATION_HELPERS_H_INCLUDED_

