// LRU - A Templated least recently used structure for threaded evaluation

// Copyright (C) 2011 Vizrt
// Released under the LGPL.

#ifndef CORE_LRU_H_
#define CORE_LRU_H_

#include <boost/thread.hpp>
#include <map>
#include <list>

namespace olib { namespace opencorelib {

template< typename Tkey, typename Tval >
class CORE_API lru
{
	public:
		typedef Tkey key_type;
		typedef Tval val_type;
		typedef std::map< key_type, val_type > map;
		typedef typename map::iterator iterator;
		typedef std::list< key_type > list;

		lru( )
		: size_( 50 )
		{
		}

		void resize( size_t size )
		{
			boost::recursive_mutex::scoped_lock lck( mutex_ );
			while( queue_.size( ) > size )
			{
				queue_.erase( queue_.find( *( lru_.begin( ) ) ) );
				lru_.pop_front( );
			}
			size_ = size;
		}

		void append( key_type index, val_type value )
		{
			boost::recursive_mutex::scoped_lock lck( mutex_ );
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

		val_type fetch( key_type index )
		{
			boost::recursive_mutex::scoped_lock lck( mutex_ );
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

		val_type wait( key_type index, boost::posix_time::time_duration time )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			val_type result;
			while( !( result = fetch( index ) ) )
				if ( !cond_.timed_wait( lock, time ) )
					break;
			return result;
		}

		void clear( )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			queue_.clear( );
			lru_.clear( );
		}

	private:
		boost::recursive_mutex mutex_;
		boost::condition_variable_any cond_;
		map queue_;
		list lru_;
		size_t size_;
};

} }

#endif
