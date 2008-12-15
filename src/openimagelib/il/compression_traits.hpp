
// il - A image library representation.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef COMPRESSION_TRAITS_INC_
#define COMPRESSION_TRAITS_INC_

namespace olib { namespace openimagelib { namespace il {

//
template<typename T, class storage = default_storage<T> >
class dxt1 : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;

private:
	static const size_type bs = 8;

public:
	explicit dxt1( size_type width, 
				   size_type height, 
				   size_type depth, 
				   size_type count		= 1, 
				   bool cubemap			= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"dxt1" )
	{ surface_format<T, storage>::allocate( ); }

	dxt1( const dxt1& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"dxt1" )
	{ surface_format<T, storage>::allocate( ); }

	virtual ~dxt1( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return detail::dds_Allocate_size<T>( )( bs, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 8; }
	
	virtual dxt1* clone( size_type w, size_type h )
	{ return new dxt1( *this, w, h ); }
};

//
template<typename T, class storage = default_storage<T> >
class dxt3 : public surface_format<T, storage>
{			
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;

private:
	static const size_type bs = 16;

public:
	explicit dxt3( size_type width, 
			       size_type height, 
			       size_type depth, 
			       size_type count	= 1, 
			       bool cubemap		= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"dxt3" )
	{ surface_format<T, storage>::allocate( ); }

	dxt3( const dxt3& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"dxt3" )
	{ surface_format<T, storage>::allocate( ); }

	virtual ~dxt3( )
	{ }
	
public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return detail::dds_Allocate_size<T>( )( bs, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 8; }
	
	virtual dxt3* clone( size_type w, size_type h )
	{ return new dxt3( *this, w, h ); }
};

//
template<typename T, class storage = default_storage<T> >
class dxt5 : public surface_format<T, storage>
{			
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;

private:
	static const size_type bs = 16;

public:
	explicit dxt5( size_type width, 
				   size_type height, 
				   size_type depth, 
				   size_type count	= 1, 
				   bool cubemap		= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"dxt5" )
	{ surface_format<T, storage>::allocate( ); }

	dxt5( const dxt5& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"dxt5" )
	{ surface_format<T, storage>::allocate( ); }
			
	virtual ~dxt5( )
	{ }
	
public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return detail::dds_Allocate_size<T>( )( bs, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 8; }
	
	virtual dxt5* clone( size_type w, size_type h )
	{ return new dxt5( *this, w, h ); }
};

} } }

#endif


