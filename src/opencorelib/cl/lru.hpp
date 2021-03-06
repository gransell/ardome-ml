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

		lru( size_t size = 50 )
		: size_( size )
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
			boost::system_time timeout = boost::get_system_time( ) + time;
			while( !( result = fetch( index ) ) )
				if ( !cond_.timed_wait( lock, timeout ) )
					break;
			return result;
		}

		/// Find an object for the index and return it, but don't change the lru state
		val_type find( key_type index ) const
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			val_type result;
			const_iterator iter = queue_.find( index );
			if ( iter != queue_.end( ) )
				result = iter->second;
			return result;
		}

		/// Wait for an object, but don't change the lru state
		val_type find( key_type index, boost::posix_time::time_duration time )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			val_type result;
			boost::system_time timeout = boost::get_system_time( ) + time;
			while( !( result = find( index ) ) )
				if ( !cond_.timed_wait( lock, timeout ) )
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
				space_.notify_all( );
			}
		}

		/// Wait for an item to be removed if the queue is full before appending
		bool append_if_not_full( key_type index, val_type value, boost::posix_time::time_duration time )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			boost::system_time timeout = boost::get_system_time( ) + time;
			while ( queue_.size( ) >= size_ )
				if ( !space_.timed_wait( lock, timeout ) )
					break;
			bool result = queue_.size( ) < size_;
			if ( result )
				append( index, value );
			return result;
		}

		/// Wait for space to become available
		bool wait_for_space( boost::posix_time::time_duration time )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			boost::system_time timeout = boost::get_system_time( ) + time;
			while ( queue_.size( ) >= size_ )
				if ( !space_.timed_wait( lock, timeout ) )
					break;
			return queue_.size( ) < size_;
		}

		/// Wait for an item to be removed
		bool wait_for_remove( boost::posix_time::time_duration time )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			boost::system_time timeout = boost::get_system_time( ) + time;
			size_t size = queue_.size( );
			while ( size != 0 && size == queue_.size( ) )
				if ( !space_.timed_wait( lock, timeout ) )
					break;
			return queue_.size( ) < size;
		}

		/// Remove all items stored
		void clear( )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			queue_.clear( );
			lru_.clear( );
			space_.notify_all( );
		}

	private:
		mutable boost::recursive_mutex mutex_;
		boost::condition_variable_any cond_;
		boost::condition_variable_any space_;
		map queue_;
		list lru_;
		size_t size_;
};

} }

#endif
