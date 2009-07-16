
#ifndef AMF_FILTER_UTILITY_H
#define AMF_FILTER_UTILITY_H

#include <ostream>
#include <boost/cstdint.hpp>

namespace amf { namespace openmedialib {

boost::int64_t parse_int64( const std::string &, int base = 10 );
boost::uint64_t parse_uint64( const std::string &, int base = 10 );

// Useful macros
#define CLAMP( v, l, u )	( v < l ? l : v > u ? u : v )
#define RANGE( v, l, u )	( v >= l && v <= u )

// Fast integer sqrt
inline int sqrti( int n )
{
	int p = 0;
	int q = 1;
	int r = n;
	int h = 0;

	while( q <= n )
		q = q << 2;

	while( q != 1 )
	{
		q = q >> 2;
		h = p + q;
		p = p >> 1;
		if ( r >= h )
		{
			p = p + q;
			r = r - h;
		}
	}

	return p;
}

// Frame audio utilities
extern void apply_volume( ml::frame_type_ptr &result, double volume, double end );
extern void extract_channel( ml::frame_type_ptr &result, int channel );
extern std::vector< double > extract_levels( ml::frame_type_ptr &result );
extern void change_pitch( ml::frame_type_ptr &result, int required );
extern bool mix_channel( ml::frame_type_ptr &result, ml::frame_type_ptr &channel, const std::vector< double > &volume, double &, int mute );
extern void join_peaks( ml::frame_type_ptr &result, std::vector< double > & );
extern void join_peaks( ml::frame_type_ptr &result, ml::frame_type_ptr &input );

// Image utilities
extern void copy_plane( il::image_type_ptr output, il::image_type_ptr input, size_t plane );
extern void fill_plane( il::image_type_ptr img, size_t plane, boost::uint8_t sample );

// Report to stream
extern void report_frame( std::ostream &stream, const ml::frame_type_ptr &frame );
extern void report_image( std::ostream &stream, const il::image_type_ptr &img, int num = 1, int den = 1 );
extern void report_alpha( std::ostream &stream, const il::image_type_ptr img );
extern void report_audio( std::ostream &stream, const ml::audio_type_ptr &audio );
extern void report_props( std::ostream &stream, const pl::pcos::property_container &props );

// Frame interogation utilities
extern void frame_report_basic( const ml::frame_type_ptr &frame );
extern void frame_report_image( const ml::frame_type_ptr &frame );
extern void frame_report_alpha( const ml::frame_type_ptr &frame );
extern void frame_report_audio( const ml::frame_type_ptr &frame );
extern void frame_report_props( const ml::frame_type_ptr &frame );

// pcos property observer
template < typename C > class fn_observer : public pcos::observer
{
	public:
		fn_observer( C *instance, void ( C::*fn )( ) )
			: instance_( instance )
			, fn_( fn )
		{ }

		virtual void updated( pcos::isubject * )
		{ ( instance_->*fn_ )( ); }

	private:
		C *instance_;
		void ( C::*fn_ )( );
};

/// Special conversion routines from wchar_t to char needed on windows
/// for use with ANSI file operations. 
extern std::string to_multibyte_string( const olib::t_string& str );

// Purge lazy objects for the specified url
extern void lazy_purge( const olib::t_string &url );
extern void lazy_purge_smaller( const olib::t_string &url, boost::int64_t new_size );

} }

#endif
