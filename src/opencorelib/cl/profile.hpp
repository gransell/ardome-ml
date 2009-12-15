
#ifndef CORE_PROFILE_H_
#define CORE_PROFILE_H_

#include "profile_types.hpp"

#include <string>
#include <vector>
#include <map>

namespace olib { namespace opencorelib {

/// Interface for the actual profile implementation
class profile
{
	public:
		/// Inner type definition
		typedef std::vector< profile_entry > list;

		/// Destructor for the profile list
		virtual ~profile( ) { }

		/// Iterator begin
		virtual list::iterator begin( ) = 0;

		/// Iterator end
		virtual list::iterator end( ) = 0;

		/// Const iterator begin
		virtual list::const_iterator begin( ) const = 0;

		/// Const iterator end
		virtual list::const_iterator end( ) const = 0;
};

/// Specify the root directory for the profiles for this process instance
extern CORE_API void profile_init( const std::string &directory );

/// Free function to load a profile
extern CORE_API profile_ptr profile_load( const std::string &profile );

/// Provides a means to wrap a C or C++ object/structure - specific members which 
/// want to be exposed should be enrolled via a name/property_profile pair and 
/// custom types can be implemented on a case by case basis
class CORE_API profile_wrapper
{
	public:
		/// Inner typedef for the wrapped properties
		typedef std::map< std::string, profile_property * > map;

		/// Constructor for the wrapper
		profile_wrapper( bool managed = true );

		/// Destructor for the wrapper
		virtual ~profile_wrapper( );

		/// Enrolls all the wrapped properties
		virtual void init( ) = 0;

		/// Work around to allow easy callbacks into objects which aren't managed (deleted) by the profile_manager
		bool managed_wrapper( ) const { return managed_; }

		/// Enroll an int property
		void enroll( const std::string &name, int &ref );

		/// Enroll an unsigned int property
		void enroll( const std::string &name, boost::uint32_t &ref );

		/// Enroll an int64 property
		void enroll( const std::string &name, boost::int64_t &ref );

		/// Enroll a float property
		void enroll( const std::string &name, float &ref );

		/// Enroll a double property
		void enroll( const std::string &name, double &ref );

		/// Enroll a custom profile property
		void enroll( const std::string &name, profile_property *property );

		/// Iterator begin
		map::iterator begin( );

		/// Iterator end
		map::iterator end( );

		/// Const iterator begin
		map::const_iterator begin( ) const;

		/// Const iterator begin
		map::const_iterator end( ) const;

		/// Find a property by name
		map::iterator find( const std::string &name );
		
	protected:
		bool managed_;
		map map_;
};

/// Holds a collection of profile wrappers and distributes a loaded profile over matching names
/// found in each instance
class CORE_API profile_manager
{
	public:
		/// Inner type definition
		typedef std::vector< profile_wrapper * > targets;

		/// Constructor for the manager
		profile_manager( );

		/// Destructor for the manager
		virtual ~profile_manager( );

		/// Register a wrapper with the manager
		void enroll( profile_wrapper *wrapper );

		/// Load a profile and distribute amongst the wrappers
		void load( const std::string &id );

	protected:
		targets targets_;
};

} }

#endif
