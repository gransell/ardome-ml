// LRU Cache - A cache mechanism for managing lru objects

// Copyright (C) 2011 Vizrt
// Released under the LGPL.

#ifndef CORE_LRU_CACHE_H_
#define CORE_LRU_CACHE_H_

#include <boost/shared_ptr.hpp>
#include "lru.hpp"
#include "lru_key.hpp"

namespace olib { namespace opencorelib {

/// Provides a templated LRU cache. 
///
/// Types should be defined as follows:
///
/// typedef lru_cache< type > lru_type_cache;
/// typedef boost::shared_ptr< type > type_ptr;
///
/// where type can be anything providing it takes no arguments in its 
/// constructor.
///
/// For example, to define an instance which holds LRU collections of 
/// frame_type_ptr objects:
///
/// typedef lru< int, ml::frame_type_ptr > lru_frame_type;
/// typedef lru_cache< lru_frame_type > lru_frame_cache;
/// typedef boost::shared_ptr< lru_frame_type > lru_frame_ptr;
///
/// The following shows a rough design of a class which uses this:
///
/// class cache_user
/// {
///   public:
///     cache_user( )
///     : key_( lru_frame_cache::allocate( ) )
///     { }
///
///     ml::frame_type_ptr fetch( int position )
///     {
///       lru_frame_ptr my_lru = lru_frame_cache::fetch( key_ );
///       ...
///       return my_lru->fetch( position );
///     }
///
///     void flush( )
///     {
///       lru_frame_cache::deallocate( key_ );
///     }
///
///   private:
///     lru_key key_;
/// };
///
/// Note that it should not be assumed the object which is being cached holds
/// its state - the fetch from the cache may return a new object. Additionally,
/// a call to deallocate does not terminate the use of the key, only the 
/// destruction of the associated object.

template< class value_type >
class CORE_API lru_cache
{
	public:
		typedef boost::shared_ptr< value_type > value_ptr;
		typedef lru< int, value_ptr > instance_type;

		/// Allocate a key
		static lru_key allocate( )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			return lru_key( key_ ++, deallocate );
		}

		/// Obtain or create an object associated to the key
		static value_ptr fetch( lru_key &key )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			instance_type *cache = instance( );
			if ( !cache->fetch( key.key( ) ) )
				cache->append( key.key( ), value_ptr( new value_type( ) ) );
			return cache->fetch( key.key( ) );
		}

		/// Destroy the currently cached object
		static void deallocate( lru_key &key )
		{
			deallocate( key.key( ) );
		}

	private:
		/// Create the instance for this type
		static instance_type *instance( )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			if ( !instance_ )
			{
				instance_ = new instance_type( );
				atexit( destroy );
			}
			return instance_;
		}

		/// Deallocate the key
		static void deallocate( int key )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			instance( )->remove( key );
		}

		/// Destroy the whole cache
		static void destroy( )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			delete instance_;
			instance_ = 0;
		}

		static boost::recursive_mutex mutex_;
		static instance_type *instance_;
		static int key_;
};

// Initialise statics in template
template < class value_type > boost::recursive_mutex lru_cache< value_type >::mutex_;
template < class value_type > lru< int, boost::shared_ptr< value_type > > *lru_cache< value_type >::instance_;
template < class value_type > int lru_cache< value_type >::key_ = 0;

} }

#endif
