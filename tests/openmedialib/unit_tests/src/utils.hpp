#ifndef UTILS_ML_TESTS_H_INCLUDED_
#define UTILS_ML_TESTS_H_INCLUDED_

#include <openmedialib/ml/stream.hpp>
#include <openmedialib/ml/types.hpp>
#include <opencorelib/cl/str_util.hpp>

#define MEDIA_REPO_PREFIX L"http://releases.ardendo.se/media-repository/"
#define MEDIA_REPO_REGRESSION_TESTS_PREFIX "http://releases.ardendo.se/media-repository/amf/RegressionTests"

//Required in order to use wstrings with BOOST_CHECK_EQUAL
namespace std {
	inline std::ostream &operator<<(std::ostream& output_stream, const std::wstring &the_string)
	{
		return output_stream << olib::opencorelib::str_util::to_string(the_string);
	}
}

// default implementation of ml::stream_type to be used in testing. Reimplement as necessary
class stream_mock : public olib::openmedialib::ml::stream_type
{
	public:
		/// Constructor for a audio packet
		stream_mock( std::string codec, size_t length, boost::int64_t position, boost::int64_t key, int bitrate,
			int frequency, int channels, int samples, int sample_size );

		/// Virtual destructor
		virtual ~stream_mock( ) ;

		virtual void set_position( boost::int64_t position );

		virtual void set_key( boost::int64_t key );

		virtual void set_index( size_t index );

		virtual size_t index( );

		/// Return the properties associated to this frame.
		virtual olib::openpluginlib::pcos::property_container &properties( );

		/// Indicates the container format of the input
		virtual const std::string &container( ) const;

		/// Indicates the codec used to decode the stream
		virtual const std::string &codec( ) const;

		/// Returns the id of the stream (so that we can select a codec to decode it)
		virtual const enum olib::openmedialib::ml::stream_id id( ) const;

		/// Returns the length of the data in the stream
		virtual size_t length( );

		/// Returns a pointer to the first byte of the stream
		virtual boost::uint8_t *bytes( );

		/// Returns the position of the key frame associated to this packet
		virtual const boost::int64_t key( ) const;

		/// Returns the position of this packet
		virtual const boost::int64_t position( ) const;

		/// Returns the bitrate of the stream this packet belongs to
		virtual const int bitrate( ) const;
		
		/// Gop size is currently unknown for avformat inputs
		virtual const int estimated_gop_size( ) const;

		/// Returns the dimensions of the image associated to this packet (0,0 if n/a)
		virtual const olib::openmedialib::ml::dimensions size( ) const ;

		/// Returns the sar of the image associated to this packet (1,1 if n/a)
		virtual const olib::openmedialib::ml::fraction sar( ) const ;

		/// Returns the frequency associated to the audio in the packet (0 if n/a)
		virtual const int frequency( ) const ;

		/// Returns the channels associated to the audio in the packet (0 if n/a)
		virtual const int channels( ) const ;

		/// Returns the samples associated to the audio in the packet (0 if n/a)
		virtual const int samples( ) const ;

		/// Returns the sample size of the audio in the packet (0 if n/a)
		virtual const int sample_size( ) const;
	
		virtual const std::wstring pf( ) const ;

		virtual olib::openimagelib::il::field_order_flags field_order( ) const;

	public:
		std::string codec_;
		enum olib::openmedialib::ml::stream_id id_;
		std::string container_;
		size_t length_;
		std::vector < boost::uint8_t > data_;
		olib::openpluginlib::pcos::property_container properties_;
		boost::int64_t position_;
		boost::int64_t key_;
		int bitrate_;
		olib::openmedialib::ml::dimensions size_;
		olib::openmedialib::ml::fraction sar_;
		int frequency_;
		int channels_;
		int samples_;
		int sample_size_;
		std::wstring pf_;
		olib::openimagelib::il::field_order_flags field_order_;
		int estimated_gop_size_;
		size_t index_;
};

typedef boost::shared_ptr< stream_mock > stream_mock_ptr;

#endif //#ifndef UTILS_ML_TESTS_H_INCLUDED_

