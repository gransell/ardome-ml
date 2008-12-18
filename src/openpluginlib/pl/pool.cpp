// pool - A pooled memory allocator

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include "pool.hpp"
using namespace std;
using namespace olib::openpluginlib;

// Declaration of mutex
OPENPLUGINLIB_DECLSPEC boost::mutex pool::mutex_;

// Constructor creates 31 pools sized at 2^i
pool::pool( )
{
}

// Destroys pools
pool::~pool( )
{
	for( int i = 0; i < 31; i ++ )
	{
		typedef std::vector< unsigned char * >::iterator iterator;
		for( iterator I = pools_[ i ].begin( ); I != pools_[ i ].end( ); ++I )
			delete[ ] *I;
	}
}

// Singleton accessor - ensures pools are destroyed on exit
pool *pool::get_instance( )
{
	static pool *instance = NULL;
	boost::mutex::scoped_lock scoped_lock( mutex_ );
	if ( instance == NULL )
	{
		instance = new pool( );
		atexit( destroy );
	}
	return instance;
}

// Singleton destructor
void pool::destroy( ) 
{ 
	delete get_instance( ); 
}

// Allocate memory >= size
unsigned char *pool::malloc( int size )
{
#ifdef HAVE_POOL
	unsigned char *result = 0;

	if ( size > 0 )
	{
		// Account for index storage
		size += sizeof(int);

		pool *self = get_instance( );
		int index = 1;
		while ( ( 1 << index ) < size ) index ++;
		boost::mutex::scoped_lock scoped_lock( mutex_ );
		int used = index;
		while ( used < 31 && self->pools_[ index ].size( ) == 0 ) used ++;
		if ( used == 31 ) used = index;
		if ( self->pools_[ used ].size( ) == 0 )
		{
			result = new unsigned char[ 1 << index ];
			if ( result != 0 )
			{
				*( int * )( result ) = index;
				result += sizeof( int );
			}
		}
		else
		{
			result = *( self->pools_[ used ].end( ) - 1 ) + sizeof( int );
			self->pools_[ used ].pop_back( );
		}
	}

	return result;
#else
	return static_cast< unsigned char * >( std::malloc( size ) );
#endif
}

// Reallocate a chunk to given size
unsigned char *pool::realloc( unsigned char *chunk, int size )
{
#ifdef HAVE_POOL
	unsigned char *result = chunk;

	if ( chunk != 0 )
	{
		// Account for index storage
		size += sizeof(int);

		int old_index = *( int * )( chunk - sizeof( int ) );
		int index = 1;
		while ( ( 1 << index ) < size ) index ++;
		if ( old_index < index )
		{
			result = pool::malloc( size );
			memcpy( result, chunk, int( 1 << old_index ) );
			pool::free( chunk );
		}
	}
	else
	{
		result = pool::malloc( size );
	}

	return result;
#else
	return static_cast< unsigned char * >( std::realloc( chunk, size ) );
#endif
}

// Free memory 
void pool::free( unsigned char *chunk )
{
#ifdef HAVE_POOL
	if ( chunk != 0 )
	{
		pool *self = get_instance( );
		int index = *( int * )( chunk - sizeof( int ) );
		boost::mutex::scoped_lock scoped_lock( mutex_ );
		self->pools_[ index ].push_back( chunk - sizeof( int ) );
	}
#else
	std::free( chunk );
#endif
}

