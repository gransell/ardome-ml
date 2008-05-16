
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef BIND_INFO_INC_
#define BIND_INFO_INC_

#ifdef HAVE_GL_GLEW_H

#ifdef _MSC_VER
#pragma warning ( push )
#pragma warning ( disable:4251 )
#endif

#if _MSC_VER >= 1400
#include <hash_map>
#else
#include <map>
#endif

#include <boost/any.hpp>

#include <openpluginlib/pl/config.hpp>
#include <openpluginlib/pl/string.hpp>

namespace olib { namespace openpluginlib {

class OPENPLUGINLIB_DECLSPEC bind_info
{
public:
	typedef wstring		key_type;
	typedef boost::any	field_type;
#if _MSC_VER >= 1400
	typedef stdext::hash_map<key_type, field_type> container;
#else
	typedef std::map<key_type, field_type> container;
#endif
	typedef container::reference		reference;
	typedef container::const_reference	const_reference;
	typedef container::value_type		value_type;

public:
	~bind_info( );
	
public:
	template<class U>
	U value( const key_type& key, const U& u_def = U( 0 ) ) const
	{
		typedef container::const_iterator const_iterator;
		
		const_iterator I = info_.find( key );
		if( I == info_.end( ) )
			return U( u_def );
		
		return boost::any_cast<U>( I->second );
	}

	template<class U>
	bool insert( const key_type& k, U v )
	{ return info_.insert( value_type( k, v ) ).second; }

private:
	container info_;
};

} }

#ifdef _MSC_VER
#pragma warning ( pop )
#endif

#endif

#endif

