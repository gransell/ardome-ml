// LRU - A Templated least recently used structure for threaded evaluation

// Copyright (C) 2011 Vizrt
// Released under the LGPL.

#ifndef CORE_LRU_H_
#define CORE_LRU_H_

#include "export_defines.hpp"

#include <boost/thread.hpp>
#include <map>
#include <list>

namespace olib { namespace opencorelib {

/// Provides a generic mechanism for controlling collections of objects based 
/// on usage - the least recently used objects will be automatically discarded
/// as new objects are added.
///
/// Notes:
///
/// * it is assumed that pointer val_type's are wrapped by a boost::shared_ptr 
///   or similar.
/// * to avoid escalation of memory usage in parts of a graph which aren't 
///   currently in use, it is advised that lru objects are obtained from the
///   lru_cache mechanism rather than instantiated directly from here.

template< typename key_type, typename val_type >
class CORE_API lru
{
	public:
		typedef std::map< key_type, val_type > map;
		typedef typename map::iterator iterator;
		typedef typename map::const_iterator const_iterator;
		typedef std::pair< const_iterator, const_iterator > pair;
		typedef std::list< key_type > list;

		lru( )
		: size_( 50 )
		{
		}

		/// Returns the number of items in the lru
		size_t count( ) const 
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			return queue_.size( );
		}

		/// Specify the maximum number of items stored
		void resize( size_t size )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			while( queue_.size( ) > size )
			{
				queue_.erase( queue_.find( *( lru_.begin( ) ) ) );
				lru_.pop_front( );
			}
			size_ = size;
		}

		/// Append an object with the specified index - becomes the most recently used
		void append( key_type index, val_type value )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			if ( queue_.find( index ) == queue_.end( ) )
			{
				queue_[ index ] = value;
				lru_.push_back( index );
				resize( size_ );
			}
			else
			{
				queue_[ index ] = value;
				fetch( index );
			}
			cond_.notify_all( );
		}

		/// Obtain an object for the specified index - becomes the most recently used when available
		val_type fetch( key_type index )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			val_type result;
			iterator iter = queue_.find( index );
			if ( iter != queue_.end( ) )
			{
				result = iter->second;
				lru_.remove( index );
				lru_.push_back( index );
			}
			return result;
		}

		/// Obtain the highest object in the specified range of indexes - the selected object becomes the most recently used
		val_type highest_in_range( key_type upper, key_type lower )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			val_type result;
			iterator iter = queue_.upper_bound( upper );
			if ( iter != queue_.begin( ) ) 
			{
				iter --;
				if ( iter->first >= lower )
				{
					result = iter->second;
					lru_.remove( iter->first );
					lru_.push_back( iter->first );
				}
			}
			return result;
		}

		/// Wait for an object of the specified index - becomes the most recently used when available
		val_type wait( key_type index, boost::posix_time::time_duration time )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			val_type result;
			while( !( result = fetch( index ) ) )
				if ( !cond_.timed_wait( lock, time ) )
					break;
			return result;
		}

		/// Remove the item with the specified index
		void remove( key_type index )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			iterator iter = queue_.find( index );
			if ( iter != queue_.end( ) )
			{
				queue_.erase( iter );
				lru_.remove( index );
			}
		}

		/// Remove all items stored
		void clear( )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			queue_.clear( );
			lru_.clear( );
		}

	private:
		mutable boost::recursive_mutex mutex_;
		boost::condition_variable_any cond_;
		map queue_;
		list lru_;
		size_t size_;
};

} }

#endif
