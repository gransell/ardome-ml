
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef REGISTRY_INC_
#define REGISTRY_INC_

#include <cstdlib>
#include <new>

#ifndef BOOST_FILESYSTEM_DYN_LINK 
    #define BOOST_FILESYSTEM_DYN_LINK 
#endif

#include <boost/filesystem/path.hpp>

#if _MSC_VER >= 1400
#include <hash_map>
#else
#include <map>
#endif

#include <openpluginlib/pl/openplugin.hpp>

namespace olib { namespace openpluginlib { namespace detail {

//
class registry
{
public:
	typedef wstring key_type;
	
#if _MSC_VER >= 1400
	typedef stdext::hash_multimap<key_type, plugin_item> container;
#else
	typedef std::multimap<key_type, plugin_item> container;
#endif

	typedef std::vector< plugin_item > list;

private:
	static registry* revive_if_dead( )
	{
		void* p = new char [ sizeof( registry ) ];

		new( p ) registry;
		
		return static_cast<registry*>( p );
	}

	static void destroy( void )
	{
		instance_->~registry( );

		delete[ ] ( ( char* ) instance_ );
	}

public:	
	static registry& instance( )
	{
		if( !instance_ )
		{
			if( destroyed_ )
			{
				destroyed_     = false;
				was_destroyed_ = true;
			}

			instance_ = revive_if_dead( );

			if( !was_destroyed_ )
				std::atexit( annihilate );
		}
		
		return *instance_;
	}

	static void annihilate( )
	{
		if( instance_ )
		{
			assert( !destroyed_ );
			destroy( );
			destroyed_ = true;
			instance_ = 0;
		}
	}

public:
	// Insert all plugins found under the lookup path into the registry's standard database
	bool insert_std( const string& lookup_path );

	// Insert all plugins found under the lookup path into the registry's custom database
	// Custom plugins are searched before standard plugins.
	bool insert_custom( const string& lookup_path );

	// TODO: Does nothing
	bool remove( );

	// Clear all our plugin dbs
	void clear( );

	const container& get_std_db( ) const
	{ return std_db_; }

	const container& get_custom_db( ) const
	{ return custom_db_; }

	const list &auto_load( ) const
	{ return auto_load_; }

	void auto_clear( )
	{ auto_load_.erase( auto_load_.begin( ), auto_load_.end( ) ); }

private:
	explicit registry( const string& lookup_path = "" );
	~registry( );
	registry( const registry& );
	registry& operator=( const registry& );
	
private:
	// Store std and custom items separatly so that they can be searched independently
	container std_db_, custom_db_;
	list auto_load_;
	static registry* instance_;
	static bool destroyed_, was_destroyed_;
};

} } }

#endif
