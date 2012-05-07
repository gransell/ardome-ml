// LRU Key - A key mechanism for managing lru objects (see lru_cache)

// Copyright (C) 2011 Vizrt
// Released under the LGPL.

#ifndef CORE_LRU_KEY_H_
#define CORE_LRU_KEY_H_

#include "export_defines.hpp"

namespace olib { namespace opencorelib {

/// Holds a unique key for referencing items in the lru cache, and a destructor
/// which is invoked when the object falls out of scope.

class CORE_API lru_key
{
	public:
		/// Type definition of the destructor
		typedef void ( * destructor )( int );

		/// Constructs a key and associates it with its destructor
		lru_key( int key, destructor dtor )
		: key_( key )
		, dtor_( dtor )
		{ }

		/// Destructs the key when the object falls out of scope
		~lru_key( )
		{ dtor_( key_ ); }

		/// Obtains the key value
		int key( ) const
		{ return key_; }

	private:
		const int key_;
		const destructor dtor_;
};

} }

#endif
