
// il - A image library representation.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef IL_YUV_TRAITS_INC_
#define IL_YUV_TRAITS_INC_

namespace olib { namespace openimagelib { namespace il {

// yuv444p - provides 3 planes - the name and size of each are shown:
// 		Y: width * height 
//		U: width * height
//		V: width * height
// Y maps 1:1 to the corresponding RGB sample at x, y
// U and V samples are mapped to the corresponding Y by fetching samples at x,y
template<typename T, class storage = default_storage<T> >
class yuv444p : public surface_format<T, storage>
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
	explicit yuv444p( size_type width, size_type height, size_type depth = 1, size_type count = 1, bool cubemap = false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"yuv444p" )
		, width_( width )
		, height_( height )
	{ surface_format<T, storage>::allocate( ); }

	yuv444p( const yuv444p& other, size_type width, size_type height )
		: surface_format<T, storage>( other.bs, width, height, other.depth( ), other.count( ), other.is_cubemap( ), L"yuv444p" )
		, width_( width )
		, height_( height )
	{ surface_format<T, storage>::allocate( ); }


	virtual ~yuv444p( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return detail::rgb_Allocate_size<T>( )( bs, width, height, depth ); }

	virtual size_type bitdepth( ) const
	{ return 8; }

	virtual yuv444p* clone( size_type w, size_type h )
	{ return new yuv444p( *this, w, h ); }

	virtual void allocplanes( planes &planes )
	{
		plane plane;

		plane.offset = 0;
		plane.pitch = width_;
		plane.width = width_;
		plane.height = height_;
		plane.linesize = width_;
		planes.push_back( plane );

		plane.offset += width_ * height_;
		planes.push_back( plane );

		plane.offset += width_ * height_;
		planes.push_back( plane );
	}

	virtual void crop_planes( planes &crop, size_type &x, size_type &y, size_type &w, size_type &h, int flags )
	{
		bool is_flipped = ( flags & flipped ) != 0;
		bool is_flopped = ( flags & flopped ) != 0;

		for ( int i = 0; i < 3; i ++ )
		{
			plane &p = crop[ i ];
			p.width = w;
			p.height = h;
			p.linesize = w;

			if ( !is_flipped )
				p.offset = p.pitch * y;
			else
				p.offset = ( height_ - y - h ) * p.pitch;

			if ( !is_flopped )
				p.offset += x;
			else
				p.offset += width_ - w - x;

			p.offset = surface_format<T, storage>::offset( i ) + p.offset;
		}
	}

	virtual void flop_scan_line( size_t, pointer dst, const_pointer src, size_type w ) const
	{
		src += w - 1;
		while( w -- )
			*dst ++ = *src --;
	}

	private:
		size_type width_;
		size_type height_;
};

// yuv444 - provides a single plane consisting of a raster of width * height * 3 bytes
// each sample is Y U V
template<typename T, class storage = default_storage<T> >
class yuv444 : public surface_format<T, storage>
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
	explicit yuv444( size_type width, size_type height, size_type depth = 1, size_type count = 1, bool cubemap = false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"yuv444" )
	{ surface_format<T, storage>::allocate( ); }

	yuv444( const yuv444& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"yuv444" )
	{ surface_format<T, storage>::allocate( ); }


	virtual ~yuv444( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{
		return detail::rgb_Allocate_size<T>( )( bs, width, height, depth ); 
	}

	virtual size_type bitdepth( ) const
	{ return 8; }

	virtual yuv444* clone( size_type w, size_type h )
	{ 
		return new yuv444( *this, w, h ); 
	}
};

// yuv422 - provides a single plane consisting of a raster of width * height * 2 bytes
// neighbouring samples on a scanline are Y0 U Y1 V
// even RGB samples are calculated from Y0 U V
// odd RGB samples are calculated from Y1 U V
template<typename T, class storage = default_storage<T> >
class yuv422 : public surface_format<T, storage>
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
	explicit yuv422( size_type width, size_type height, size_type depth = 1, size_type count = 1, bool cubemap = false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"yuv422" )
	{ surface_format<T, storage>::allocate( ); }

	yuv422( const yuv422& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"yuv422" )
	{ surface_format<T, storage>::allocate( ); }

	virtual ~yuv422( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{
		return detail::rgb_Allocate_size<T>( )( bs, width, height, depth ); 
	}

	virtual size_type bitdepth( ) const
	{ return 8; }

	virtual yuv422* clone( size_type w, size_type h )
	{ 
		return new yuv422( *this, w, h ); 
	}
};

// uyv422 - provides a single plane consisting of a raster of width * height * 2 bytes
// neighbouring samples on a scanline are U Y0 V Y1
// even RGB samples are calculated from Y0 U V
// odd RGB samples are calculated from Y1 U V
template<typename T, class storage = default_storage<T> >
class uyv422 : public surface_format<T, storage>
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
	explicit uyv422( size_type width, size_type height, size_type depth = 1, size_type count = 1, bool cubemap = false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"uyv422" )
	{ surface_format<T, storage>::allocate( ); }

	uyv422( const uyv422& other, size_type w, size_type h )
		: surface_format<T, storage>( other.bs, w, h, other.depth( ), other.count( ), other.is_cubemap( ), L"uyv422" )
	{ surface_format<T, storage>::allocate( ); }

	virtual ~uyv422( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{
		return detail::rgb_Allocate_size<T>( )( bs, width, height, depth ); 
	}

	virtual size_type bitdepth( ) const
	{ return 8; }

	virtual uyv422* clone( size_type w, size_type h )
	{ 
		return new uyv422( *this, w, h ); 
	}
};

// yuv422p - provides a 3 planes - the name and size of each are shown:
// 		Y: width * height 
//		U: width * height / 2 
//		V: width * height / 2
// Y maps 1:1 to the corresponding RGB sample at x, y
// U and V samples are mapped to the corresponding Y by fetching samples at x/2,y
template<typename T, class storage = default_storage<T> >
class yuv422p : public surface_format<T, storage>
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
	explicit yuv422p( size_type width, size_type height, size_type depth = 1, size_type count = 1, bool cubemap = false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"yuv422p" )
		, width_( width )
		, height_( height )
	{ surface_format<T, storage>::allocate( ); }

	yuv422p( const yuv422p& other, size_type width, size_type height )
		: surface_format<T, storage>( other.bs, width, height, other.depth( ), other.count( ), other.is_cubemap( ), L"yuv422p" )
		, width_( width )
		, height_( height )
	{ surface_format<T, storage>::allocate( ); }

	virtual ~yuv422p( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{
		return detail::rgb_Allocate_size<T>( )( bs, width, height, depth ); 
	}

	virtual size_type bitdepth( ) const
	{ return 8; }

	virtual yuv422p* clone( size_type w, size_type h )
	{ 
		return new yuv422p( *this, w, h ); 
	}

	virtual void allocplanes( planes &planes )
	{
		plane plane;

		plane.offset = 0;
		plane.pitch = width_;
		plane.width = width_;
		plane.height = height_;
		plane.linesize = width_;
		planes.push_back( plane );

		plane.offset += width_ * height_;
		plane.pitch = width_ / 2;
		plane.width = width_ / 2;
		plane.height = height_;
		plane.linesize = width_ / 2;
		planes.push_back( plane );

		plane.offset += ( width_ * height_ ) / 2;
		planes.push_back( plane );
	}

	virtual void crop_planes( planes &crop, size_type &x, size_type &y, size_type &w, size_type &h, int flags )
	{
		bool is_flipped = ( flags & flipped ) != 0;
		bool is_flopped = ( flags & flopped ) != 0;

		for ( int i = 0; i < 3; i ++ )
		{
			int factor = i > 0 ? 2 : 1;

			plane &p = crop[ i ];
			int original = p.offset;
			p.width = w / factor;
			p.height = h;
			p.linesize = w / factor;

			if ( !is_flipped )
				p.offset = p.pitch * y;
			else
				p.offset = ( ( height_ - y - h ) * p.pitch );

			if ( !is_flopped )
				p.offset += x;
			else
				p.offset += ( width_ - w - x );

			p.offset = original + p.offset;
		}
	}

	virtual void flop_scan_line( size_t, pointer dst, const_pointer src, size_type w ) const
	{
		src += w - 1;
		while( w -- )
		{
			*dst ++ = *src ++;
			src -= 2;
		}
	}

	private:
		size_type width_;
		size_type height_;
};

// yuv420p - provides 3 planes - the name and size of each are shown:
// 		Y: width * height 
//		U: width * height / 4 
//		V: width * height / 4
// Y maps 1:1 to the corresponding RGB sample at x, y
// U and V samples are mapped to the corresponding Y by fetching samples at x/2,y/2
template<typename T, class storage = default_storage<T> >
class yuv420p : public surface_format<T, storage>
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
	explicit yuv420p( size_type width, size_type height, size_type depth = 1, size_type count = 1, bool cubemap = false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"yuv420p" )
		, width_( width )
		, height_( height )
	{ surface_format<T, storage>::allocate( ); }

	yuv420p( const yuv420p& other, size_type width, size_type height )
		: surface_format<T, storage>( other.bs, width, height, other.depth( ), other.count( ), other.is_cubemap( ), L"yuv420p" )
		, width_( width )
		, height_( height )
	{ surface_format<T, storage>::allocate( ); }


	virtual ~yuv420p( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{
		return detail::rgb_Allocate_size<T>( )( bs, width, height, depth ) / 2; 
	}

	virtual size_type bitdepth( ) const
	{ return 8; }

	virtual yuv420p* clone( size_type w, size_type h )
	{ 
		return new yuv420p( *this, w, h ); 
	}

	virtual void allocplanes( planes &planes )
	{
		plane plane;

		plane.offset = 0;
		plane.pitch = width_;
		plane.width = width_;
		plane.height = height_;
		plane.linesize = width_;
		planes.push_back( plane );

		plane.offset += width_ * height_;
		plane.pitch = width_ / 2;
		plane.width = width_ / 2;
		plane.height = height_ / 2;
		plane.linesize = width_ / 2;
		planes.push_back( plane );

		plane.offset += ( width_ * height_ ) / 4;
		planes.push_back( plane );
	}

	virtual void crop_planes( planes &crop, size_type &x, size_type &y, size_type &w, size_type &h, int flags )
	{
		bool is_flipped = ( flags & flipped ) != 0;
		bool is_flopped = ( flags & flopped ) != 0;

		for ( int i = 0; i < 3; i ++ )
		{
			int factor = i > 0 ? 2 : 1;

			plane &p = crop[ i ];
			p.width = w / factor;
			p.height = h / factor;
			p.linesize = w / factor;

			if ( !is_flipped )
				p.offset = p.pitch * ( y / factor );
			else
				p.offset = ( ( height_ - y - h ) * p.pitch ) / factor;

			if ( !is_flopped )
				p.offset += ( x / factor );
			else
				p.offset += ( width_ - w - x ) / factor;

			p.offset = surface_format<T, storage>::offset( i ) + p.offset;
		}
	}

	virtual void flop_scan_line( size_t, pointer dst, const_pointer src, size_type w ) const
	{
		src += w - 1;
		while( w -- )
		{
			*dst ++ = *src ++;
			src -= 2;
		}
	}

	private:
		size_type width_;
		size_type height_;
};

// yuv411 - provides a single plane consisting of a raster of ( width * height * 3 ) / 2 bytes
// neighbouring samples on a scanline are Y0 Y1 U Y2 Y3 V
// x % 4 = 0 RGB samples are calculated from Y0 U V
// x % 4 = 1 RGB samples are calculated from Y1 U V
// x % 4 = 2 RGB samples are calculated from Y2 U V
// x % 4 = 3 RGB samples are calculated from Y3 U V
template<typename T, class storage = default_storage<T> >
class yuv411 : public surface_format<T, storage>
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
	explicit yuv411( size_type width, size_type height, size_type depth = 1, size_type count = 1, bool cubemap = false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"yuv411" )
		, width_( width )
		, height_( height )
	{ surface_format<T, storage>::allocate( ); }

	yuv411( const yuv411& other, size_type width, size_type height )
		: surface_format<T, storage>( other.bs, width, height, other.depth( ), other.count( ), other.is_cubemap( ), L"yuv411" )
		, width_( width )
		, height_( height )
	{ surface_format<T, storage>::allocate( ); }

	virtual ~yuv411( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{
		return detail::rgb_Allocate_size<T>( )( bs, width, height, depth ) / 2; 
	}

	virtual size_type bitdepth( ) const
	{ return 8; }

	virtual yuv411* clone( size_type w, size_type h )
	{ 
		return new yuv411( *this, w, h ); 
	}

	virtual void allocplanes( planes &planes )
	{
		plane plane;

		plane.offset = 0;
		plane.pitch = width_ * 3 / 2;
		plane.width = width_;
		plane.height = height_;
		plane.linesize = width_ * 3 / 2;
		planes.push_back( plane );
	}

	virtual void crop_planes( planes &crop, size_type &x, size_type &y, size_type &w, size_type &h, int flags )
	{
		bool is_flipped = ( flags & flipped ) != 0;
		bool is_flopped = ( flags & flopped ) != 0;

		plane &p = crop[ 0 ];
		p.width = w;
		p.height = h;
		p.linesize = w * 3 / 2;

		if ( !is_flipped )
			p.offset = p.pitch * y;
		else
			p.offset = ( ( height_ - y - h ) * p.pitch );

		if ( !is_flopped )
			p.offset += x * 3 / 2;
		else
			p.offset += ( width_ - w - x ) * 3 / 2;
	}

	virtual void flop_scan_line( size_t, pointer dst, const_pointer src, size_type w ) const
	{
		w /= 4;
		src += w - 6;
		while( w -- )
		{
			*dst ++ = *src ++;
			*dst ++ = *src ++;
			*dst ++ = *src ++;
			*dst ++ = *src ++;
			*dst ++ = *src ++;
			*dst ++ = *src ++;
			src -= 12;
		}
	}

	private:
		size_type width_;
		size_type height_;
};

// yuv411p - provides a 3 planes - the name and size of each are shown:
// 		Y: width * height 
//		U: width * height / 4 
//		V: width * height / 4
// Y maps 1:1 to the corresponding RGB sample at x, y
// U and V samples are mapped to the corresponding Y by fetching samples at x/2,y/2
template<typename T, class storage = default_storage<T> >
class yuv411p : public surface_format<T, storage>
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
	explicit yuv411p( size_type width, size_type height, size_type depth = 1, size_type count = 1, bool cubemap = false )
		: surface_format<T, storage>( bs, width, height, depth, count, cubemap, L"yuv411p" )
		, width_( width )
		, height_( height )
	{ surface_format<T, storage>::allocate( ); }

	yuv411p( const yuv411p& other, size_type width, size_type height )
		: surface_format<T, storage>( other.bs, width, height, other.depth( ), other.count( ), other.is_cubemap( ), L"yuv411p" )
		, width_( width )
		, height_( height )
	{ surface_format<T, storage>::allocate( ); }

	virtual ~yuv411p( )
	{ }

public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const
	{
		return detail::rgb_Allocate_size<T>( )( bs, width, height, depth ) / 2; 
	}

	virtual size_type bitdepth( ) const
	{ return 8; }

	virtual yuv411p* clone( size_type w, size_type h )
	{ 
		return new yuv411p( *this, w, h ); 
	}

	virtual void allocplanes( planes &planes )
	{
		plane plane;

		plane.offset = 0;
		plane.pitch = width_;
		plane.width = width_;
		plane.height = height_;
		plane.linesize = width_;
		planes.push_back( plane );

		plane.offset += width_ * height_;
		plane.pitch = width_ / 4;
		plane.width = width_ / 4;
		plane.height = height_;
		plane.linesize = width_ / 4;
		planes.push_back( plane );

		plane.offset += ( width_ * height_ ) / 4;
		planes.push_back( plane );
	}

	virtual void crop_planes( planes &crop, size_type &x, size_type &y, size_type &w, size_type &h, int flags )
	{
		bool is_flipped = ( flags & flipped ) != 0;
		bool is_flopped = ( flags & flopped ) != 0;

		for ( int i = 0; i < 3; i ++ )
		{
			int factor = i > 0 ? 4 : 1;

			plane &p = crop[ i ];
			p.width = w / factor;
			p.height = h;
			p.linesize = w / factor;

			if ( !is_flipped )
				p.offset = p.pitch * y;
			else
				p.offset = ( ( height_ - y - h ) * p.pitch ) / factor;

			if ( !is_flopped )
				p.offset += x;
			else
				p.offset += ( width_ - w - x ) / factor;

			p.offset = surface_format<T, storage>::offset( i ) + p.offset;
		}
	}

	virtual void flop_scan_line( size_t, pointer dst, const_pointer src, size_type w ) const
	{
		src += w - 1;
		while( w -- )
		{
			*dst ++ = *src ++;
			src -= 2;
		}
	}

	private:
		size_type width_;
		size_type height_;
};

} } }

#endif
