
// il - A image library representation.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef RGB_TRAITS_INC_
#define RGB_TRAITS_INC_

namespace olib { namespace openimagelib { namespace il {

//
template<typename T, class storage = default_storage<T> >
class b8g8r8 : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;

private:
	static const int bs = 3;

public:
	explicit b8g8r8( size_type width, 
					 size_type height, 
					 size_type depth, 
					 size_type count		= 1, 
					 bool cubemap			= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"b8g8r8" )
	{ surface_format<T, storage>::allocate( ); }

	b8g8r8( const b8g8r8& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"b8g8r8" )
	{ surface_format<T, storage>::allocate( ); }

	virtual ~b8g8r8( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return detail::rgb_Allocate_size<T>( )( bs, width, height, depth ); }
	
	virtual size_type bitdepth( ) const
	{ return 8; }
	
	virtual b8g8r8* clone( size_type w, size_type h )
	{ return new b8g8r8( *this, w, h ); }
};

//
template<typename T, class storage = default_storage<T> >
class r8g8b8 : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;

private:
	static const int bs = 3;

public:
	explicit r8g8b8( size_type width, 
					 size_type height, 
					 size_type depth, 
					 size_type count		= 1, 
					 bool cubemap			= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"r8g8b8" )
	{ surface_format<T, storage>::allocate( ); }

	r8g8b8( const r8g8b8& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"r8g8b8" )
	{ surface_format<T, storage>::allocate( ); }

	virtual ~r8g8b8( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return detail::rgb_Allocate_size<T>( )( bs, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 8; }
	
	virtual r8g8b8* clone( size_type w, size_type h )
	{ return new r8g8b8( *this, w, h ); }
};

//
template<typename T, class storage = default_storage<T> >
class r8g8b8p : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;
	typedef typename surface_format<T, storage>::plane		plane;
	typedef typename surface_format<T, storage>::planes		planes;

private:
	static const int bs = 3;

public:
	explicit r8g8b8p( size_type width, 
					  size_type height, 
					  size_type depth, 
					  size_type count		= 1, 
					  bool cubemap			= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"r8g8b8p" )
		, width_( width )
		, height_( height )
	{ surface_format<T, storage>::allocate( ); }

	r8g8b8p( const r8g8b8p& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"r8g8b8p" )
		, width_( w )
		, height_( h )		
	{ surface_format<T, storage>::allocate( ); }

	virtual ~r8g8b8p( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return bs * detail::rgb_Allocate_size<T>( )( 1, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 8; }
	
	virtual r8g8b8p* clone( size_type w, size_type h )
	{ return new r8g8b8p( *this, w, h ); }
	
	virtual void allocplanes( planes& planes )
	{
		plane plane;

		plane.offset = 0;
		plane.pitch = ( ( width_ + 3 ) & -4 ) * sizeof( T );
		plane.width = width_;
		plane.height = height_;
		plane.linesize = width_ * sizeof( T );
		planes.push_back( plane );

		plane.offset += plane.pitch * height_;
		planes.push_back( plane );

		plane.offset *= 2;
		planes.push_back( plane );
	}
	
private:
	size_type width_, height_;
};

//
template<typename T, class storage = default_storage<T> >
class r8g8b8a8p : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;
	typedef typename surface_format<T, storage>::plane		plane;
	typedef typename surface_format<T, storage>::planes		planes;

private:
	static const int bs = 4;

public:
	explicit r8g8b8a8p( size_type width, 
					  size_type height, 
					  size_type depth, 
					  size_type count		= 1, 
					  bool cubemap			= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"r8g8b8a8p" )
		, width_( width )
		, height_( height )
	{ surface_format<T, storage>::allocate( ); }

	r8g8b8a8p( const r8g8b8a8p& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"r8g8b8a8p" )
		, width_( w )
		, height_( h )		
	{ surface_format<T, storage>::allocate( ); }

	virtual ~r8g8b8a8p( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return bs * detail::rgb_Allocate_size<T>( )( 1, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 8; }
	
	virtual r8g8b8a8p* clone( size_type w, size_type h )
	{ return new r8g8b8a8p( *this, w, h ); }
	
	virtual void allocplanes( planes& planes )
	{
		plane plane;

		plane.offset = 0;
		plane.pitch = ( ( width_ + 3 ) & -4 ) * sizeof( T );
		plane.width = width_;
		plane.height = height_;
		plane.linesize = width_ * sizeof( T );
		planes.push_back( plane );

		plane.offset += plane.pitch * height_;
		planes.push_back( plane );

		plane.offset *= 2;
		planes.push_back( plane );
		
		plane.offset *= 2;
		planes.push_back( plane );
	}
	
private:
	size_type width_, height_;
};

//
template<typename T, class storage = default_storage<T> >
class b8g8r8a8 : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;

private:
	static const int bs = 4;

public:
	explicit b8g8r8a8( size_type width, 
					   size_type height, 
					   size_type depth, 
					   size_type count		= 1, 
					   bool cubemap			= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"b8g8r8a8" )
	{ surface_format<T, storage>::allocate( ); }

	b8g8r8a8( const b8g8r8a8& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"b8g8r8a8" )
	{ surface_format<T, storage>::allocate( ); }
		
	virtual ~b8g8r8a8( )
	{ }
	
public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return detail::rgb_Allocate_size<T>( )( bs, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 8; }
	
	virtual b8g8r8a8* clone( size_type w, size_type h )
	{ return new b8g8r8a8( *this, w, h ); }
};

//
template<typename T, class storage = default_storage<T> >
class a8b8g8r8 : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;

private:
	static const int bs = 4;

public:
	explicit a8b8g8r8( size_type width, 
					   size_type height, 
					   size_type depth, 
					   size_type count		= 1, 
					   bool cubemap			= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"a8b8g8r8" )
	{ surface_format<T, storage>::allocate( ); }

	a8b8g8r8( const a8b8g8r8& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"a8b8g8r8" )
	{ surface_format<T, storage>::allocate( ); }
		
	virtual ~a8b8g8r8( )
	{ }
	
public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return detail::rgb_Allocate_size<T>( )( bs, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 8; }
	
	virtual a8b8g8r8* clone( size_type w, size_type h )
	{ return new a8b8g8r8( *this, w, h ); }
};

template<typename T, class storage = default_storage<T> >
class r8g8b8a8 : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;

private:
	static const int bs = 4;

public:
	explicit r8g8b8a8( size_type width, 
					   size_type height, 
					   size_type depth, 
					   size_type count		= 1, 
					   bool cubemap			= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"r8g8b8a8" )
	{ surface_format<T, storage>::allocate( ); }

	r8g8b8a8( const r8g8b8a8& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"r8g8b8a8" )
	{ surface_format<T, storage>::allocate( ); }
		
	virtual ~r8g8b8a8( )
	{ }
	
public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return detail::rgb_Allocate_size<T>( )( bs, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 8; }
	
	virtual r8g8b8a8* clone( size_type w, size_type h )
	{ return new r8g8b8a8( *this, w, h ); }
};

//
// surface format to represent L8 format.
//
template<typename T, class storage = default_storage<T> >
class l8 : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;
	
private:
	static const int bs = 1;
		
public:
	explicit l8( size_type width,
				 size_type height,
				 size_type depth,
				 size_type count		= 1,
				 bool cubemap			= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"l8" )
	{ surface_format<T, storage>::allocate( ); }

	l8( const l8& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"l8" )
	{ surface_format<T, storage>::allocate( ); }
		
	virtual ~l8( )
	{ }
	
public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return detail::rgb_Allocate_size<T>( )( bs, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 8; }
	
	virtual l8* clone( size_type w, size_type h )
	{ return new l8( *this, w, h ); }
};

//
// surface format to represent L8A8 format.
//
template<typename T, class storage = default_storage<T> >
class l8a8 : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;
	
private:
	static const int bs = 2;
		
public:
	explicit l8a8( size_type width,
				   size_type height,
				   size_type depth,
				   size_type count		= 1,
				   bool cubemap			= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"l8a8" )
	{ surface_format<T, storage>::allocate( ); }

	l8a8( const l8a8& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"l8a8" )
	{ surface_format<T, storage>::allocate( ); }
		
	virtual ~l8a8( )
	{ }
	
public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return detail::rgb_Allocate_size<T>( )( bs, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 8; }
	
	virtual l8a8* clone( size_type w, size_type h )
	{ return new l8a8( *this, w, h ); }
};

template<typename T, class storage = default_storage<T> >
class l8a8p : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;
	typedef typename surface_format<T, storage>::plane		plane;
	typedef typename surface_format<T, storage>::planes		planes;
	
private:
	static const int bs = 2;
		
public:
	explicit l8a8p( size_type width,
				   size_type height,
				   size_type depth,
				   size_type count		= 1,
				   bool cubemap			= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"l8a8p" )
		, width_( width )
		, height_( height )
	{ surface_format<T, storage>::allocate( ); }

	l8a8p( const l8a8p& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"l8a8p" )
		, width_( w )
		, height_( h )
	{ surface_format<T, storage>::allocate( ); }

	virtual ~l8a8p( )
	{ }
	
public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return bs * detail::rgb_Allocate_size<T>( )( 1, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 8; }
	
	virtual l8a8p* clone( size_type w, size_type h )
	{ return new l8a8p( *this, w, h ); }

	virtual void allocplanes( planes& planes )
	{
		plane plane;

		plane.offset = 0;
		plane.pitch = ( ( width_ + 3 ) & -4 ) * sizeof( T );
		plane.width = width_;
		plane.height = height_;
		plane.linesize = width_ * sizeof( T );
		planes.push_back( plane );

		plane.offset += plane.pitch * height_;
		planes.push_back( plane );
	}
	
private:
	size_type width_, height_;
};

template<typename T, class storage = default_storage<T> >
class l12a12p : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;
	typedef typename surface_format<T, storage>::plane		plane;
	typedef typename surface_format<T, storage>::planes		planes;

private:
	static const int bs = 2;

public:
	explicit l12a12p( size_type width, 
					 size_type height, 
					 size_type depth, 
					 size_type count		= 1, 
					 bool cubemap			= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"l12a12p" )
		, width_( width )
		, height_( height )
	{ surface_format<T, storage>::allocate( ); }

	l12a12p( const l12a12p& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"l12a12p" )
		, width_( w )
		, height_( h )
	{ surface_format<T, storage>::allocate( ); }

	virtual ~l12a12p( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return bs * sizeof( unsigned short ) * detail::rgb_Allocate_size<T>( )( 1, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 12; }
	
	virtual l12a12p* clone( size_type w, size_type h )
	{ return new l12a12p( *this, w, h ); }

	virtual void allocplanes( planes& planes )
	{
		plane plane;

		plane.offset = 0;
		plane.pitch = ( ( width_ + 3 ) & -4 ) * sizeof( T );
		plane.width = width_;
		plane.height = height_;
		plane.linesize = width_ * sizeof( T );
		planes.push_back( plane );
		
		plane.offset += plane.pitch * sizeof( unsigned short ) * height_;
		planes.push_back( plane );
	}

private:
	size_type width_, height_;
};

template<typename T, class storage = default_storage<T> >
class l16a16p : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;
	typedef typename surface_format<T, storage>::plane		plane;
	typedef typename surface_format<T, storage>::planes		planes;

private:
	static const int bs = 2;

public:
	explicit l16a16p( size_type width, 
					 size_type height, 
					 size_type depth, 
					 size_type count		= 1, 
					 bool cubemap			= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"l16a16p" )
		, width_( width )
		, height_( height )
	{ surface_format<T, storage>::allocate( ); }

	l16a16p( const l16a16p& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"l16a16p" )
		, width_( w )
		, height_( h )
	{ surface_format<T, storage>::allocate( ); }

	virtual ~l16a16p( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return bs * sizeof( unsigned short ) * detail::rgb_Allocate_size<T>( )( 1, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 16; }
	
	virtual l16a16p* clone( size_type w, size_type h )
	{ return new l16a16p( *this, w, h ); }

	virtual void allocplanes( planes& planes )
	{
		plane plane;

		plane.offset = 0;
		plane.pitch = ( ( width_ + 3 ) & -4 ) * sizeof( T );
		plane.width = width_;
		plane.height = height_;
		plane.linesize = width_ * sizeof( T );
		planes.push_back( plane );
		
		plane.offset += plane.pitch * sizeof( unsigned short ) * height_;
		planes.push_back( plane );
	}

private:
	size_type width_, height_;
};

//
template<typename T, class storage = default_storage<T> >
class a8r8g8b8 : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;

private:
	static const int bs = 4;

public:
	explicit a8r8g8b8( size_type width, 
					   size_type height, 
					   size_type depth, 
					   size_type count		= 1, 
					   bool cubemap			= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"a8r8g8b8" )
	{ surface_format<T, storage>::allocate( ); }

	a8r8g8b8( const a8r8g8b8& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"a8r8g8b8" )
	{ surface_format<T, storage>::allocate( ); }

	virtual ~a8r8g8b8( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return detail::rgb_Allocate_size<T>( )( bs, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 8; }
	
	virtual a8r8g8b8* clone( size_type w, size_type h )
	{ return new a8r8g8b8( *this, w, h ); }
};

template<typename T, class storage = default_storage<T> >
class r10g10b10 : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;
	typedef typename surface_format<T, storage>::plane		plane;
	typedef typename surface_format<T, storage>::planes		planes;

private:
	static const int bs = 3;

public:
	explicit r10g10b10( size_type width, 
					 size_type height, 
					 size_type depth, 
					 size_type count		= 1, 
					 bool cubemap			= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"r10g10b10" )
		, width_( width )
		, height_( height )
	{ surface_format<T, storage>::allocate( ); }

	r10g10b10( const r10g10b10& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"r10g10b10" )
		, width_( w )
		, height_( h )
	{ surface_format<T, storage>::allocate( ); }

	virtual ~r10g10b10( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return sizeof( unsigned short ) * detail::rgb_Allocate_size<T>( )( bs, width, height, depth ); }
	
	virtual size_type bitdepth( ) const
	{ return 10; }

	virtual r10g10b10* clone( size_type w, size_type h )
	{ return new r10g10b10( *this, w, h ); }

	virtual void allocplanes( planes& planes )
	{
		plane plane;

		plane.offset = 0;
		plane.pitch = ( ( width_ * bs + 3 ) & -4 ) * sizeof( T );
		plane.width = width_;
		plane.height = height_;
		plane.linesize = width_ * bs * sizeof( T );
		planes.push_back( plane );
	}

private:
	size_type width_, height_;
};

template<typename T, class storage = default_storage<T> >
class r10g10b10a10 : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;
	typedef typename surface_format<T, storage>::plane		plane;
	typedef typename surface_format<T, storage>::planes		planes;

private:
	static const int bs = 4;

public:
	explicit r10g10b10a10( size_type width, 
					 size_type height, 
					 size_type depth, 
					 size_type count		= 1, 
					 bool cubemap			= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"r10g10b10a10" )
		, width_( width )
		, height_( height )
	{ surface_format<T, storage>::allocate( ); }

	r10g10b10a10( const r10g10b10a10& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"r10g10b10a10" )
		, width_( w )
		, height_( h )
	{ surface_format<T, storage>::allocate( ); }

	virtual ~r10g10b10a10( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return sizeof( unsigned short ) * detail::rgb_Allocate_size<T>( )( bs, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 10; }

	virtual r10g10b10a10* clone( size_type w, size_type h )
	{ return new r10g10b10a10( *this, w, h ); }

	virtual void allocplanes( planes& planes )
	{
		plane plane;

		plane.offset = 0;
		plane.pitch = ( ( width_ * bs + 3 ) & -4 ) * sizeof( T );
		plane.width = width_;
		plane.height = height_;
		plane.linesize = width_ * bs * sizeof( T );
		planes.push_back( plane );
	}

private:
	size_type width_, height_;
};

//
template<typename T, class storage = default_storage<T> >
class r10g10b10p : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;
	typedef typename surface_format<T, storage>::plane		plane;
	typedef typename surface_format<T, storage>::planes		planes;

private:
	static const int bs = 3;

public:
	explicit r10g10b10p( size_type width, 
					  size_type height, 
					  size_type depth, 
					  size_type count		= 1, 
					  bool cubemap			= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"r10g10b10p" )
		, width_( width )
		, height_( height )
	{ surface_format<T, storage>::allocate( ); }

	r10g10b10p( const r10g10b10p& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"r10g10b10p" )
		, width_( w )
		, height_( h )		
	{ surface_format<T, storage>::allocate( ); }

	virtual ~r10g10b10p( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return bs * sizeof( unsigned short ) * detail::rgb_Allocate_size<T>( )( 1, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 10; }
	
	virtual r10g10b10p* clone( size_type w, size_type h )
	{ return new r10g10b10p( *this, w, h ); }
	
	virtual void allocplanes( planes& planes )
	{
		plane plane;

		plane.offset = 0;
		plane.pitch = ( ( ( width_ + 3 ) & -4 ) * sizeof( T ) );
		plane.width = width_;
		plane.height = height_;
		plane.linesize = width_ * sizeof( T );
		planes.push_back( plane );

		plane.offset += plane.pitch * sizeof( unsigned short ) * height_;
		planes.push_back( plane );

		plane.offset *= 2;
		planes.push_back( plane );
	}
	
private:
	size_type width_, height_;
};

//
template<typename T, class storage = default_storage<T> >
class r10g10b10a10p : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;
	typedef typename surface_format<T, storage>::plane		plane;
	typedef typename surface_format<T, storage>::planes		planes;

private:
	static const int bs = 4;

public:
	explicit r10g10b10a10p( size_type width, 
					  size_type height, 
					  size_type depth, 
					  size_type count		= 1, 
					  bool cubemap			= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"r10g10b10a10p" )
		, width_( width )
		, height_( height )
	{ surface_format<T, storage>::allocate( ); }

	r10g10b10a10p( const r10g10b10a10p& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"r10g10b10a10p" )
		, width_( w )
		, height_( h )		
	{ surface_format<T, storage>::allocate( ); }

	virtual ~r10g10b10a10p( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return bs * sizeof( unsigned short ) * detail::rgb_Allocate_size<T>( )( 1, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 10; }
	
	virtual r10g10b10a10p* clone( size_type w, size_type h )
	{ return new r10g10b10a10p( *this, w, h ); }
	
	virtual void allocplanes( planes& planes )
	{
		plane plane;

		plane.offset = 0;
		plane.pitch = ( ( width_ + 3 ) & -4 ) * sizeof( T );
		plane.width = width_;
		plane.height = height_;
		plane.linesize = width_ * sizeof( T );
		planes.push_back( plane );

		plane.offset += plane.pitch * sizeof( unsigned short ) * height_;
		planes.push_back( plane );

		plane.offset *= 2;
		planes.push_back( plane );
		
		plane.offset *= 2;
		planes.push_back( plane );
	}
	
private:
	size_type width_, height_;
};

template<typename T, class storage = default_storage<T> >
class r12g12b12 : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;
	typedef typename surface_format<T, storage>::plane		plane;
	typedef typename surface_format<T, storage>::planes		planes;

private:
	static const int bs = 3;

public:
	explicit r12g12b12( size_type width, 
					 size_type height, 
					 size_type depth, 
					 size_type count		= 1, 
					 bool cubemap			= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"r12g12b12" )
		, width_( width )
		, height_( height )
	{ surface_format<T, storage>::allocate( ); }

	r12g12b12( const r12g12b12& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"r12g12b12" )
		, width_( w )
		, height_( h )
	{ surface_format<T, storage>::allocate( ); }

	virtual ~r12g12b12( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return sizeof( unsigned short ) * detail::rgb_Allocate_size<T>( )( bs, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 12; }
	
	virtual r12g12b12* clone( size_type w, size_type h )
	{ return new r12g12b12( *this, w, h ); }

	virtual void allocplanes( planes& planes )
	{
		plane plane;

		plane.offset = 0;
		plane.pitch = ( ( width_ * bs + 3 ) & -4 ) * sizeof( T );
		plane.width = width_;
		plane.height = height_;
		plane.linesize = width_ * bs * sizeof( T );
		planes.push_back( plane );
	}

private:
	size_type width_, height_;
};

template<typename T, class storage = default_storage<T> >
class r12g12b12a12 : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;
	typedef typename surface_format<T, storage>::plane		plane;
	typedef typename surface_format<T, storage>::planes		planes;

private:
	static const int bs = 4;

public:
	explicit r12g12b12a12( size_type width, 
					 size_type height, 
					 size_type depth, 
					 size_type count		= 1, 
					 bool cubemap			= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"r12g12b12a12" )
		, width_( width )
		, height_( height )
	{ surface_format<T, storage>::allocate( ); }

	r12g12b12a12( const r12g12b12a12& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"r12g12b12a12" )
		, width_( w )
		, height_( h )
	{ surface_format<T, storage>::allocate( ); }

	virtual ~r12g12b12a12( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return sizeof( unsigned short ) * detail::rgb_Allocate_size<T>( )( bs, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 12; }
	
	virtual r12g12b12a12* clone( size_type w, size_type h )
	{ return new r12g12b12a12( *this, w, h ); }

	virtual void allocplanes( planes& planes )
	{
		plane plane;

		plane.offset = 0;
		plane.pitch = ( ( width_ * bs + 3 ) & -4 ) * sizeof( T );
		plane.width = width_;
		plane.height = height_;
		plane.linesize = width_ * bs * sizeof( T );
		planes.push_back( plane );
	}

private:
	size_type width_, height_;
};

//
template<typename T, class storage = default_storage<T> >
class r12g12b12p : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;
	typedef typename surface_format<T, storage>::plane		plane;
	typedef typename surface_format<T, storage>::planes		planes;

private:
	static const int bs = 3;

public:
	explicit r12g12b12p( size_type width, 
					  size_type height, 
					  size_type depth, 
					  size_type count		= 1, 
					  bool cubemap			= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"r12g12b12p" )
		, width_( width )
		, height_( height )
	{ surface_format<T, storage>::allocate( ); }

	r12g12b12p( const r12g12b12p& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"r12g12b12p" )
		, width_( w )
		, height_( h )		
	{ surface_format<T, storage>::allocate( ); }

	virtual ~r12g12b12p( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return bs * sizeof( unsigned short ) * detail::rgb_Allocate_size<T>( )( 1, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 12; }
	
	virtual r12g12b12p* clone( size_type w, size_type h )
	{ return new r12g12b12p( *this, w, h ); }
	
	virtual void allocplanes( planes& planes )
	{
		plane plane;

		plane.offset = 0;
		plane.pitch = ( ( width_ + 3 ) & -4 ) * sizeof( T );
		plane.width = width_;
		plane.height = height_;
		plane.linesize = width_ * sizeof( T );
		planes.push_back( plane );

		plane.offset += plane.pitch * sizeof( unsigned short ) * height_;
		planes.push_back( plane );

		plane.offset *= 2;
		planes.push_back( plane );
	}
	
private:
	size_type width_, height_;
};

//
template<typename T, class storage = default_storage<T> >
class r12g12b12a12p : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;
	typedef typename surface_format<T, storage>::plane		plane;
	typedef typename surface_format<T, storage>::planes		planes;

private:
	static const int bs = 4;

public:
	explicit r12g12b12a12p( size_type width, 
					  size_type height, 
					  size_type depth, 
					  size_type count		= 1, 
					  bool cubemap			= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"r12g12b12a12p" )
		, width_( width )
		, height_( height )
	{ surface_format<T, storage>::allocate( ); }

	r12g12b12a12p( const r12g12b12a12p& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"r12g12b12a12p" )
		, width_( w )
		, height_( h )		
	{ surface_format<T, storage>::allocate( ); }

	virtual ~r12g12b12a12p( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return bs * sizeof( unsigned short ) * detail::rgb_Allocate_size<T>( )( 1, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 12; }
	
	virtual r12g12b12a12p* clone( size_type w, size_type h )
	{ return new r12g12b12a12p( *this, w, h ); }
	
	virtual void allocplanes( planes& planes )
	{
		plane plane;

		plane.offset = 0;
		plane.pitch = ( ( width_ + 3 ) & -4 ) * sizeof( T );
		plane.width = width_;
		plane.height = height_;
		plane.linesize = width_ * sizeof( T );
		planes.push_back( plane );

		plane.offset += plane.pitch * sizeof( unsigned short ) * height_;
		planes.push_back( plane );

		plane.offset *= 2;
		planes.push_back( plane );
		
		plane.offset *= 2;
		planes.push_back( plane );
	}
	
private:
	size_type width_, height_;
};

template<typename T, class storage = default_storage<T> >
class r16g16b16 : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;
	typedef typename surface_format<T, storage>::plane		plane;
	typedef typename surface_format<T, storage>::planes		planes;

private:
	static const int bs = 3;

public:
	explicit r16g16b16( size_type width, 
					 size_type height, 
					 size_type depth, 
					 size_type count		= 1, 
					 bool cubemap			= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"r16g16b16" )
		, width_( width )
		, height_( height )
	{ surface_format<T, storage>::allocate( ); }

	r16g16b16( const r16g16b16& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"r16g16b16" )
		, width_( w )
		, height_( h )
	{ surface_format<T, storage>::allocate( ); }

	virtual ~r16g16b16( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return sizeof( unsigned short ) * detail::rgb_Allocate_size<T>( )( bs, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 16; }
	
	virtual r16g16b16* clone( size_type w, size_type h )
	{ return new r16g16b16( *this, w, h ); }

	virtual void allocplanes( planes& planes )
	{
		plane plane;

		plane.offset = 0;
		plane.pitch = ( ( width_ * bs + 3 ) & -4 ) * sizeof( T );
		plane.width = width_;
		plane.height = height_;
		plane.linesize = width_ * bs * sizeof( T );
		planes.push_back( plane );
	}

private:
	size_type width_, height_;
};

template<typename T, class storage = default_storage<T> >
class r16g16b16a16 : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;
	typedef typename surface_format<T, storage>::plane		plane;
	typedef typename surface_format<T, storage>::planes		planes;

private:
	static const int bs = 4;

public:
	explicit r16g16b16a16( size_type width, 
					   size_type height, 
					   size_type depth, 
					   size_type count		= 1, 
					   bool cubemap			= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"r16g16b16a16" )
		, width_( width )
		, height_( height )
	{ surface_format<T, storage>::allocate( ); }

	r16g16b16a16( const r16g16b16a16& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"r16g16b16a16" )
		, width_( w )
		, height_( h )
	{ surface_format<T, storage>::allocate( ); }
		
	virtual ~r16g16b16a16( )
	{ }
	
public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return sizeof( unsigned short ) * detail::rgb_Allocate_size<T>( )( bs, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 16; }
	
	virtual r16g16b16a16* clone( size_type w, size_type h )
	{ return new r16g16b16a16( *this, w, h ); }
	
	virtual void allocplanes( planes& planes )
	{
		plane plane;

		plane.offset = 0;
		plane.pitch = ( ( width_ * bs + 3 ) & -4 ) * sizeof( T );
		plane.width = width_;
		plane.height = height_;
		plane.linesize = width_ * bs * sizeof( T );
		planes.push_back( plane );
	}

private:
	size_type width_, height_;
};

//
template<typename T, class storage = default_storage<T> >
class r16g16b16p : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;
	typedef typename surface_format<T, storage>::plane		plane;
	typedef typename surface_format<T, storage>::planes		planes;

private:
	static const int bs = 3;

public:
	explicit r16g16b16p( size_type width, 
					  size_type height, 
					  size_type depth, 
					  size_type count		= 1, 
					  bool cubemap			= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"r16g16b16p" )
		, width_( width )
		, height_( height )
	{ surface_format<T, storage>::allocate( ); }

	r16g16b16p( const r16g16b16p& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"r16g16b16p" )
		, width_( w )
		, height_( h )		
	{ surface_format<T, storage>::allocate( ); }

	virtual ~r16g16b16p( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return bs * sizeof( unsigned short ) * detail::rgb_Allocate_size<T>( )( 1, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 16; }
	
	virtual r16g16b16p* clone( size_type w, size_type h )
	{ return new r16g16b16p( *this, w, h ); }
	
	virtual void allocplanes( planes& planes )
	{
		plane plane;

		plane.offset = 0;
		plane.pitch = ( ( width_ + 3 ) & -4 ) * sizeof( T );
		plane.width = width_;
		plane.height = height_;
		plane.linesize = width_ * sizeof( T );
		planes.push_back( plane );

		plane.offset += plane.pitch * sizeof( unsigned short ) * height_;
		planes.push_back( plane );

		plane.offset *= 2;
		planes.push_back( plane );
	}
	
private:
	size_type width_, height_;
};

//
template<typename T, class storage = default_storage<T> >
class r16g16b16a16p : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;
	typedef typename surface_format<T, storage>::plane		plane;
	typedef typename surface_format<T, storage>::planes		planes;

private:
	static const int bs = 4;

public:
	explicit r16g16b16a16p( size_type width, 
					  size_type height, 
					  size_type depth, 
					  size_type count		= 1, 
					  bool cubemap			= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"r16g16b16a16p" )
		, width_( width )
		, height_( height )
	{ surface_format<T, storage>::allocate( ); }

	r16g16b16a16p( const r16g16b16a16p& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"r16g16b16a16p" )
		, width_( w )
		, height_( h )		
	{ surface_format<T, storage>::allocate( ); }

	virtual ~r16g16b16a16p( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return bs * sizeof( unsigned short ) * detail::rgb_Allocate_size<T>( )( 1, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 16; }
	
	virtual r16g16b16a16p* clone( size_type w, size_type h )
	{ return new r16g16b16a16p( *this, w, h ); }
	
	virtual void allocplanes( planes& planes )
	{
		plane plane;

		plane.offset = 0;
		plane.pitch = ( ( width_ + 3 ) & -4 ) * sizeof( T );
		plane.width = width_;
		plane.height = height_;
		plane.linesize = width_ * sizeof( T );
		planes.push_back( plane );

		plane.offset += plane.pitch * sizeof( unsigned short ) * height_;
		planes.push_back( plane );

		plane.offset *= 2;
		planes.push_back( plane );
		
		plane.offset *= 2;
		planes.push_back( plane );
	}
	
private:
	size_type width_, height_;
};

// printing density format traits

template<typename T, class storage = default_storage<T> >
class r8g8b8log : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;
	typedef typename surface_format<T, storage>::plane		plane;
	typedef typename surface_format<T, storage>::planes		planes;

private:
	static const int bs = 3;

public:
	explicit r8g8b8log( size_type width, 
					    size_type height, 
					    size_type depth, 
					    size_type count	= 1, 
					    bool cubemap		= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"r8g8b8log" )
		, width_( width )
		, height_( height )
	{ surface_format<T, storage>::allocate( ); }

	r8g8b8log( const r8g8b8log& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"r8g8b8log" )
		, width_( w )
		, height_( h )		
	{ surface_format<T, storage>::allocate( ); }

	virtual ~r8g8b8log( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return detail::rgb_Allocate_size<T>( )( bs, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 8; }
	
	virtual r8g8b8log* clone( size_type w, size_type h )
	{ return new r8g8b8log( *this, w, h ); }

	virtual void allocplanes( planes& planes )
	{
		plane plane;

		plane.offset = 0;
		plane.pitch = ( ( width_ * bs + 3 ) & -4 ) * sizeof( T );
		plane.width = width_;
		plane.height = height_;
		plane.linesize = width_ * bs * sizeof( T );
		planes.push_back( plane );
	}
	
private:
	size_type width_, height_;
};

template<typename T, class storage = default_storage<T> >
class r10g10b10log : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;
	typedef typename surface_format<T, storage>::plane		plane;
	typedef typename surface_format<T, storage>::planes		planes;

private:
	static const int bs = 3;

public:
	explicit r10g10b10log( size_type width, 
						   size_type height, 
						   size_type depth, 
						   size_type count	= 1, 
						   bool cubemap		= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"r10g10b10log" )
		, width_( width )
		, height_( height )
	{ surface_format<T, storage>::allocate( ); }

	r10g10b10log( const r10g10b10log& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"r10g10b10log" )
		, width_( w )
		, height_( h )		
	{ surface_format<T, storage>::allocate( ); }

	virtual ~r10g10b10log( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return sizeof( unsigned short ) * detail::rgb_Allocate_size<T>( )( bs, width, height, depth ); }
	
	virtual size_type bitdepth( ) const
	{ return 10; }

	virtual r10g10b10log* clone( size_type w, size_type h )
	{ return new r10g10b10log( *this, w, h ); }

	virtual void allocplanes( planes& planes )
	{
		plane plane;

		plane.offset = 0;
		plane.pitch = ( ( width_ * bs + 3 ) & -4 ) * sizeof( T );
		plane.width = width_;
		plane.height = height_;
		plane.linesize = width_ * bs * sizeof( T );
		planes.push_back( plane );
	}
	
private:
	size_type width_, height_;
};

template<typename T, class storage = default_storage<T> >
class r12g12b12log : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;
	typedef typename surface_format<T, storage>::plane		plane;
	typedef typename surface_format<T, storage>::planes		planes;

private:
	static const int bs = 3;

public:
	explicit r12g12b12log( size_type width, 
						   size_type height, 
						   size_type depth, 
						   size_type count	= 1, 
						   bool cubemap		= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"r12g12b12log" )
		, width_( width )
		, height_( height )
	{ surface_format<T, storage>::allocate( ); }

	r12g12b12log( const r12g12b12log& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"r12g12b12log" )
		, width_( w )
		, height_( h )		
	{ surface_format<T, storage>::allocate( ); }

	virtual ~r12g12b12log( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return sizeof( unsigned short ) * detail::rgb_Allocate_size<T>( )( bs, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 12; }
	
	virtual r12g12b12log* clone( size_type w, size_type h )
	{ return new r12g12b12log( *this, w, h ); }

	virtual void allocplanes( planes& planes )
	{
		plane plane;

		plane.offset = 0;
		plane.pitch = ( ( width_ * bs + 3 ) & -4 ) * sizeof( T );
		plane.width = width_;
		plane.height = height_;
		plane.linesize = width_ * bs * sizeof( T );
		planes.push_back( plane );
	}
	
private:
	size_type width_, height_;
};

template<typename T, class storage = default_storage<T> >
class r16g16b16log : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;
	typedef typename surface_format<T, storage>::plane		plane;
	typedef typename surface_format<T, storage>::planes		planes;

private:
	static const int bs = 3;

public:
	explicit r16g16b16log( size_type width, 
						   size_type height, 
						   size_type depth, 
						   size_type count	= 1, 
						   bool cubemap		= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"r16g16b16log" )
		, width_( width )
		, height_( height )
	{ surface_format<T, storage>::allocate( ); }

	r16g16b16log( const r16g16b16log& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"r16g16b16log" )
		, width_( w )
		, height_( h )		
	{ surface_format<T, storage>::allocate( ); }

	virtual ~r16g16b16log( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return sizeof( unsigned short ) * detail::rgb_Allocate_size<T>( )( bs, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 16; }
	
	virtual r16g16b16log* clone( size_type w, size_type h )
	{ return new r16g16b16log( *this, w, h ); }

	virtual void allocplanes( planes& planes )
	{
		plane plane;

		plane.offset = 0;
		plane.pitch = ( ( width_ * bs + 3 ) & -4 ) * sizeof( T );
		plane.width = width_;
		plane.height = height_;
		plane.linesize = width_ * bs * sizeof( T );
		planes.push_back( plane );
	}
	
private:
	size_type width_, height_;
};

template<typename T, class storage = default_storage<T> >
class r8g8b8a8log : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;
	typedef typename surface_format<T, storage>::plane		plane;
	typedef typename surface_format<T, storage>::planes		planes;

private:
	static const int bs = 3;

public:
	explicit r8g8b8a8log( size_type width, 
					    size_type height, 
					    size_type depth, 
					    size_type count	= 1, 
					    bool cubemap		= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"r8g8b8a8log" )
		, width_( width )
		, height_( height )
	{ surface_format<T, storage>::allocate( ); }

	r8g8b8a8log( const r8g8b8a8log& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"r8g8b8a8log" )
		, width_( w )
		, height_( h )		
	{ surface_format<T, storage>::allocate( ); }

	virtual ~r8g8b8a8log( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return detail::rgb_Allocate_size<T>( )( bs, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 8; }
	
	virtual r8g8b8a8log* clone( size_type w, size_type h )
	{ return new r8g8b8a8log( *this, w, h ); }

	virtual void allocplanes( planes& planes )
	{
		plane plane;

		plane.offset = 0;
		plane.pitch = ( ( width_ * bs + 3 ) & -4 ) * sizeof( T );
		plane.width = width_;
		plane.height = height_;
		plane.linesize = width_ * bs * sizeof( T );
		planes.push_back( plane );
	}
	
private:
	size_type width_, height_;
};

template<typename T, class storage = default_storage<T> >
class r10g10b10a10log : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;
	typedef typename surface_format<T, storage>::plane		plane;
	typedef typename surface_format<T, storage>::planes		planes;

private:
	static const int bs = 3;

public:
	explicit r10g10b10a10log( size_type width, 
						   size_type height, 
						   size_type depth, 
						   size_type count	= 1, 
						   bool cubemap		= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"r10g10b10a10log" )
		, width_( width )
		, height_( height )
	{ surface_format<T, storage>::allocate( ); }

	r10g10b10a10log( const r10g10b10a10log& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"r10g10b10a10log" )
		, width_( w )
		, height_( h )		
	{ surface_format<T, storage>::allocate( ); }

	virtual ~r10g10b10a10log( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return sizeof( unsigned short ) * detail::rgb_Allocate_size<T>( )( bs, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 10; }
	
	virtual r10g10b10a10log* clone( size_type w, size_type h )
	{ return new r10g10b10a10log( *this, w, h ); }

	virtual void allocplanes( planes& planes )
	{
		plane plane;

		plane.offset = 0;
		plane.pitch = ( ( width_ * bs + 3 ) & -4 ) * sizeof( T );
		plane.width = width_;
		plane.height = height_;
		plane.linesize = width_ * bs * sizeof( T );
		planes.push_back( plane );
	}
	
private:
	size_type width_, height_;
};

template<typename T, class storage = default_storage<T> >
class r12g12b12a12log : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;
	typedef typename surface_format<T, storage>::plane		plane;
	typedef typename surface_format<T, storage>::planes		planes;

private:
	static const int bs = 4;

public:
	explicit r12g12b12a12log( size_type width, 
						   size_type height, 
						   size_type depth, 
						   size_type count	= 1, 
						   bool cubemap		= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"r12g12b12a12log" )
		, width_( width )
		, height_( height )
	{ surface_format<T, storage>::allocate( ); }

	r12g12b12a12log( const r12g12b12a12log& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"r12g12b12a12log" )
		, width_( w )
		, height_( h )		
	{ surface_format<T, storage>::allocate( ); }

	virtual ~r12g12b12a12log( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return sizeof( unsigned short ) * detail::rgb_Allocate_size<T>( )( bs, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 12; }
	
	virtual r12g12b12a12log* clone( size_type w, size_type h )
	{ return new r12g12b12a12log( *this, w, h ); }

	virtual void allocplanes( planes& planes )
	{
		plane plane;

		plane.offset = 0;
		plane.pitch = ( ( width_ * bs + 3 ) & -4 ) * sizeof( T );
		plane.width = width_;
		plane.height = height_;
		plane.linesize = width_ * bs * sizeof( T );
		planes.push_back( plane );
	}
	
private:
	size_type width_, height_;
};

template<typename T, class storage = default_storage<T> >
class r16g16b16a16log : public surface_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename surface_format<T, storage>::size_type	size_type;
	typedef typename surface_format<T, storage>::plane		plane;
	typedef typename surface_format<T, storage>::planes		planes;

private:
	static const int bs = 4;

public:
	explicit r16g16b16a16log( size_type width, 
						   size_type height, 
						   size_type depth, 
						   size_type count	= 1, 
						   bool cubemap		= false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"r16g16b16a16log" )
		, width_( width )
		, height_( height )
	{ surface_format<T, storage>::allocate( ); }

	r16g16b16a16log( const r16g16b16a16log& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"r16g16b16a16log" )
		, width_( w )
		, height_( h )		
	{ surface_format<T, storage>::allocate( ); }

	virtual ~r16g16b16a16log( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return sizeof( unsigned short ) * detail::rgb_Allocate_size<T>( )( bs, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 16; }
	
	virtual r16g16b16a16log* clone( size_type w, size_type h )
	{ return new r16g16b16a16log( *this, w, h ); }

	virtual void allocplanes( planes& planes )
	{
		plane plane;

		plane.offset = 0;
		plane.pitch = ( ( width_ * bs + 3 ) & -4 ) * sizeof( T );
		plane.width = width_;
		plane.height = height_;
		plane.linesize = width_ * bs * sizeof( T );
		planes.push_back( plane );
	}
	
private:
	size_type width_, height_;
};

} } }

#endif
