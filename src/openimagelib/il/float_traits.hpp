
// il - A image library representation.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef FLOAT_TRAITS_INC_
#define FLOAT_TRAITS_INC_

namespace olib { namespace openimagelib { namespace il {

//
// surface format to represent Greg Ward's RGBE format.
//
template<typename T = float, class storage = default_storage<T> >
class rgbe : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;

private:
	static const size_type bs = 4;

public:
	explicit rgbe( size_type width,
				   size_type height,
				   size_type depth,
				   size_type count		= 1,
				   bool cubemap			= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"rgbe" )
	{ surface_format<T, storage>::allocate( ); }

	rgbe( const rgbe& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"rgbe" )
	{ surface_format<T, storage>::allocate( ); }

	~rgbe( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return detail::rgb_Allocate_size<T>( )( bs, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 8; }

	virtual bool is_float( ) const { return true; }
	
	virtual rgbe* clone( size_type w, size_type h )
	{ return new rgbe( *this, w, h ); }
};

template<typename T = float, class storage = default_storage<T> >
class r16g16b16f : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;

private:
	static const size_type bs = 3;

public:
	explicit r16g16b16f( size_type width,
						 size_type height,
						 size_type depth,
						 size_type count	= 1,
						 bool cubemap		= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"r16g16b16f" )
	{ surface_format<T, storage>::allocate( ); }

	r16g16b16f( const r16g16b16f& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"r16g16b16f" )
	{ surface_format<T, storage>::allocate( ); }

	~r16g16b16f( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return sizeof( unsigned short ) * detail::rgb_Allocate_size<T>( )( bs, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 16; }
	
	virtual bool is_float( ) const { return true; }
	
	virtual r16g16b16f* clone( size_type w, size_type h )
	{ return new r16g16b16f( *this, w, h ); }
};

template<typename T = float, class storage = default_storage<T> >
class b16g16r16f : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;

private:
	static const size_type bs = 3;

public:
	explicit b16g16r16f( size_type width,
						 size_type height,
						 size_type depth,
						 size_type count	= 1,
						 bool cubemap		= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"b16g16r16f" )
	{ surface_format<T, storage>::allocate( ); }

	b16g16r16f( const b16g16r16f& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"b16g16r16f" )
	{ surface_format<T, storage>::allocate( ); }

	~b16g16r16f( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return sizeof( unsigned short ) * detail::rgb_Allocate_size<T>( )( bs, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 16; }
	
	virtual bool is_float( ) const { return true; }
	
	virtual b16g16r16f* clone( size_type w, size_type h )
	{ return new b16g16r16f( *this, w, h ); }
};

template<typename T = float, class storage = default_storage<T> >
class r16g16b16a16f : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;

private:
	static const size_type bs = 4;

public:
	explicit r16g16b16a16f( size_type width,
						 size_type height,
						 size_type depth,
						 size_type count	= 1,
						 bool cubemap		= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"r16g16b16a16f" )
	{ surface_format<T, storage>::allocate( ); }

	r16g16b16a16f( const r16g16b16a16f& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"r16g16b16a16f" )
	{ surface_format<T, storage>::allocate( ); }

	~r16g16b16a16f( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return sizeof( unsigned short ) * detail::rgb_Allocate_size<T>( )( bs, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 16; }

	virtual bool is_float( ) const { return true; }

	virtual r16g16b16a16f* clone( size_type w, size_type h )
	{ return new r16g16b16a16f( *this, w, h ); }
};

template<typename T = float, class storage = default_storage<T> >
class r32g32b32f : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;

private:
	static const size_type bs = 3;

public:
	explicit r32g32b32f( size_type width,
						 size_type height,
						 size_type depth,
						 size_type count	= 1,
						 bool cubemap		= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"r32g32b32f" )
	{ surface_format<T, storage>::allocate( ); }

	r32g32b32f( const r32g32b32f& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"r32g32b32f" )
	{ surface_format<T, storage>::allocate( ); }

	~r32g32b32f( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return sizeof( float ) * detail::rgb_Allocate_size<T>( )( bs, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 32; }
	
	virtual bool is_float( ) const { return true; }
	
	virtual r32g32b32f* clone( size_type w, size_type h )
	{ return new r32g32b32f( *this, w, h ); }
};

template<typename T = float, class storage = default_storage<T> >
class r32g32b32a32f : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;

private:
	static const size_type bs = 4;

public:
	explicit r32g32b32a32f( size_type width,
						 size_type height,
						 size_type depth,
						 size_type count	= 1,
						 bool cubemap		= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"r32g32b32a32f" )
	{ surface_format<T, storage>::allocate( ); }

	r32g32b32a32f( const r32g32b32a32f& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"r32g32b32a32f" )
	{ surface_format<T, storage>::allocate( ); }

	~r32g32b32a32f( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return sizeof( float ) * detail::rgb_Allocate_size<T>( )( bs, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 32; }

	virtual bool is_float( ) const { return true; }
	
	virtual r32g32b32a32f* clone( size_type w, size_type h )
	{ return new r32g32b32a32f( *this, w, h ); }
};

} } }

#endif
