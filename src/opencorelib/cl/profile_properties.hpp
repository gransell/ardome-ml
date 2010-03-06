
#ifndef CORE_PROFILE_PROPERTIES_H_
#define CORE_PROFILE_PROPERTIES_H_

#include "profile_types.hpp"

#include <opencorelib/cl/core.hpp>
#include <opencorelib/cl/enforce_defines.hpp>

#include <string>
#include <sstream>
#include <stdexcept>

namespace olib { namespace opencorelib {

template< typename T >
struct larger_type
{
	typedef boost::int64_t type;
};

template< >
struct larger_type< boost::uint64_t >
{
	typedef boost::uint64_t type;
};


/// Templated form for ints
template< typename T >
class CORE_API profile_number : public profile_property
{
	public:
		profile_number( T &ref )
			: ref_( ref )
		{ }

		virtual ~profile_number( )
		{ }

		void assign( const std::string &name, const profile_op &op, const std::string &value )
		{
			T temp = deserialise( value );
			switch( op )
			{
				case op_equals:
					ref_ = temp;
					break;
				case op_or:
					ref_ |= temp;
					break;
				case op_and:
					ref_ &= temp;
					break;
				case op_off:
					ref_ &= ~temp;
					break;
				case op_xor:
					ref_ ^= temp;
					break;
			}
		}

		std::string serialise( const std::string & ) const
		{
			std::ostringstream oss;
			oss << ref_;
			return oss.str( );
		}

	protected:
		T deserialise( const std::string &value ) const
		{
			std::istringstream iss;
			iss.str( value );
			if( value.find( 'x' ) != std::string::npos )
			{
				iss >> std::hex;
			}

			//Needed to parse 0x80000000 and larger into an int
			typename larger_type<T>::type temp;
			iss >> temp;
			
			ARENFORCE_MSG( !iss.fail(), "Could not interpret profile value string \"%1%\"" )( value );

			return static_cast<T>(temp);
		}

		T &ref_;
};

/// Templated form for floats
template< typename T >
class CORE_API profile_floating : public profile_property
{
	public:
		profile_floating( T &ref )
			: ref_( ref )
		{ }

		virtual ~profile_floating( )
		{ }

		void assign( const std::string &name, const profile_op &op, const std::string &value )
		{
			T temp = deserialise( value );
			switch( op )
			{
				case op_equals:
					ref_ = temp;
					break;
				case op_or:
				case op_and:
				case op_off:
				case op_xor:
					throw std::runtime_error( "Invalid operation for floating point" );
					break;
			}
		}

		std::string serialise( const std::string & ) const
		{
			std::ostringstream oss;
			oss << ref_;
			return oss.str( );
		}

	protected:
		T deserialise( const std::string &value ) const
		{
			T temp;
			std::istringstream iss;
			iss.str( value );
			iss >> temp;
			return temp;
		}

		T &ref_;
};

/// Templated form for strings
template< typename T >
class CORE_API profile_strings : public profile_property
{
	public:
		profile_strings( T &ref )
			: ref_( ref )
		{ }

		virtual ~profile_strings( )
		{ }

		void assign( const std::string &name, const profile_op &op, const std::string &value )
		{
			T temp = deserialise( value );
			switch( op )
			{
				case op_equals:
					ref_ = temp;
					break;
				case op_or:
				case op_and:
				case op_off:
				case op_xor:
					throw std::runtime_error( "Invalid operation for floating point" );
					break;
			}
		}

		std::string serialise( const std::string & ) const
		{
			return ref_;
		}

	protected:
		T deserialise( const std::string &value ) const
		{
			return value;
		}

		T &ref_;
};

} }

#endif
