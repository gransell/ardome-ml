#include "precompiled_headers.hpp"

#include "serialization_helpers.hpp"
#include "detail/base64_conversions.hpp"

#include <opencorelib/cl/log_defines.hpp>
#include <opencorelib/cl/enforce_defines.hpp>
#include <opencorelib/cl/central_plugin_factory.hpp>
#include <opencorelib/cl/serializer.hpp>
#include <opencorelib/cl/deserializer.hpp>

using std::ios_base;
using std::istream;
using std::istream_iterator;
using std::ostream;
using std::ostream_iterator;

namespace olib{ namespace opencorelib{



	CORE_API void serialize_to_stream( object_ptr obj, std::ostream &stream, const olib::t_string &serializer_plugin, serialization_mode::type mode )
	{
		ARSCOPELOG_DEBUG5( );

		ARLOG_DEBUG5( "Creating serializer %1%" )( serializer_plugin );
		serializer_ptr sr;
		ARENFORCE_MSG( sr = create_plugin<serializer>( serializer_plugin ), "Could not create serializer %1%")(serializer_plugin);
		if( mode == serialization_mode::base64 )
		{
			std::stringstream temp_stream;
			sr->serialize( temp_stream, obj );
			detail::base64_encode( istream_iterator<char>(temp_stream), istream_iterator<char>(), ostream_iterator<boost::uint8_t>(stream) );
		}
		else
		{
			sr->serialize( stream, obj );
		}
	}

	CORE_API t_string serialize_to_string( object_ptr obj, const olib::t_string &serializer_plugin, serialization_mode::type mode )
	{
		std::stringstream result;
		serialize_to_stream( obj, result, serializer_plugin, mode );

		return str_util::to_t_string( result.str() );
	}

	CORE_API void serialize_to_path( object_ptr obj, const olib::t_path &filepath, const olib::t_string &serializer_plugin, serialization_mode::type mode )
	{
		std::ofstream ofs( filepath.c_str(), ( mode == serialization_mode::binary ? ios_base::out | ios_base::binary : ios_base::out ) );
		ARENFORCE_MSG(ofs.good(), "Failed to create output stream to file \"%1%\"")( filepath.native() );

		serialize_to_stream( obj, ofs, serializer_plugin, mode );
	}

	CORE_API object_ptr deserialize_from_stream( std::istream &stream, const olib::t_string &deserializer_plugin, serialization_mode::type mode )
	{
		ARSCOPELOG_DEBUG5( );

		ARLOG_DEBUG5( "Creating deserializer %1%" )( deserializer_plugin );

		deserializer_ptr dsr;
		ARENFORCE_MSG( dsr = create_plugin< deserializer >( deserializer_plugin ), "Could not create deserializer %1%" )( deserializer_plugin );

		object_ptr result;

		if( mode == serialization_mode::base64 )
		{
			std::stringstream temp_stream;
			detail::base64_decode( istream_iterator<char>(stream), istream_iterator<char>(), ostream_iterator<boost::uint8_t>(temp_stream) );
			result = dsr->deserialize( temp_stream );
		}
		else
		{
			result = dsr->deserialize( stream );
		}

		ARENFORCE_MSG( result, "Deserialization with plugin %1% failed." )( deserializer_plugin );

		return result;
	}

	CORE_API object_ptr deserialize_from_string( const olib::t_string &str, const olib::t_string &deserializer_plugin, serialization_mode::type mode )
	{
		std::stringstream temp( str_util::to_string( str ) );

		return deserialize_from_stream( temp, deserializer_plugin, mode );
	}

	CORE_API object_ptr deserialize_from_path( const olib::t_path &filepath, const olib::t_string &deserializer_plugin, serialization_mode::type mode )
	{
		std::ifstream ifs( filepath.c_str(), ( mode == serialization_mode::binary ? ios_base::in | ios_base::binary : ios_base::in ) );
		ARENFORCE_MSG( ifs.good(), "Failed to open file %1% for reading." )( filepath.native() );

		return deserialize_from_stream( ifs, deserializer_plugin, mode );
	}


} }

