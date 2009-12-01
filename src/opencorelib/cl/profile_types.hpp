
#ifndef CORE_PROFILE_TYPES_H_
#define CORE_PROFILE_TYPES_H_

#include "export_defines.hpp"

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>

namespace olib { namespace opencorelib {

/// Shared pointer wrapper for the profile list
typedef boost::shared_ptr< class profile > profile_ptr;

/// A propery provides the evaluation and assignment logic
class profile_property;

/// Holds a collection of properties and the associated onject to which the properties belong
class profile_wrapper;

/// Holds a collection of wrappers and distribute profile assignments
class profile_manager;

/// Forward declaration to property templates
template< typename T > class profile_number;
template< typename T > class profile_floating;
template< typename T > class profile_strings;

/// Implementations of various standard types
typedef profile_number< int > profile_int;
typedef profile_number< boost::uint32_t > profile_uint;
typedef profile_number< boost::int64_t > profile_int64;
typedef profile_number< boost::uint64_t > profile_uint64;
typedef profile_floating< float > profile_float;
typedef profile_floating< double > profile_double;
typedef profile_strings< std::string > profile_string;

/// Enumerated type of assignment operators
enum profile_op
{
	op_equals,
	op_or,
	op_and,
	op_off,
	op_xor
};

/// The definition of the name operator value structure
struct profile_entry
{
	std::string name;
	enum profile_op op;
	std::string value;
};

/// Abstract class which defines a profile property (a mapping between the profile name and the reference to a variable to assign)
class CORE_API profile_property
{
	public:
		/// Virual destructor
		virtual ~profile_property( ) 
		{ }

		/// Abstract call with a profile_entry as input
		void assign( const profile_entry &entry ) { assign( entry.name, entry.op, entry.value ); }

		/// Pure virtual assignment method
		virtual void assign( const std::string &, const profile_op &, const std::string & ) = 0;

		/// String serialisation courtesy function
		virtual std::string serialise( const std::string & ) const = 0;

		/// Work around to allow easy callbacks into objects which aren't managed (deleted) by the profile_wrapper
		virtual const bool managed_property( ) const { return true; }
};

} }

#endif
