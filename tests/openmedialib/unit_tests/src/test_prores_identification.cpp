#include <boost/test/unit_test.hpp>
#include <openmedialib/ml/utilities.hpp>
#include <openmedialib/ml/input.hpp>
#include <openmedialib/ml/frame.hpp>
#include "utils.hpp"

using namespace olib::openmedialib::ml;
using namespace olib::opencorelib::str_util;

namespace
{
	void test( const std::wstring &media_file,
		const std::string &expected_codec_suffix,
		const std::string &expected_pixel_format )
	{
		const std::wstring full_uri( L"avformat:" MEDIA_REPO_PREFIX_W L"MOV/ProRes/ShortTests/" + media_file );

		input_type_ptr avformat_input = create_delayed_input( full_uri );
		avformat_input->property( "packet_stream" ) = 1;
		BOOST_REQUIRE( avformat_input->init() );

		frame_type_ptr frame = avformat_input->fetch();
		BOOST_REQUIRE( frame );
		stream_type_ptr stream = frame->get_stream();
		BOOST_REQUIRE( stream );

		BOOST_CHECK_EQUAL( stream->codec(), "http://www.ardendo.com/apf/codec/" + expected_codec_suffix );
		BOOST_CHECK_EQUAL( to_string( stream->pf() ), expected_pixel_format );

		BOOST_CHECK_EQUAL( stream->estimated_gop_size( ), 1 );
	}
}

BOOST_AUTO_TEST_SUITE( test_prores_identification )

BOOST_AUTO_TEST_CASE( prores_422 )
{
	test( L"ProRes422_1080i50_0ch.mov",        "prores/422", "yuv422p10le" );
	test( L"ProRes422_1080i59.94-DF_0ch.mov",  "prores/422", "yuv422p10le" );
	test( L"ProRes422_1080i59.94-NDF_0ch.mov", "prores/422", "yuv422p10le" );
	test( L"ProRes422_1080p24_0ch.mov",        "prores/422", "yuv422p10le" );
	test( L"ProRes422_1080p25_0ch.mov",        "prores/422", "yuv422p10le" );
	test( L"ProRes422_1080p30_0ch.mov",        "prores/422", "yuv422p10le" );
	test( L"ProRes422_1080p50_0ch.mov",        "prores/422", "yuv422p10le" );
	test( L"ProRes422_1080p60_0ch.mov",        "prores/422", "yuv422p10le" );
}

BOOST_AUTO_TEST_CASE( prores_422_hq )
{
	test( L"ProRes422_HQ_1080i50_0ch.mov",        "prores/422_hq", "yuv422p10le" );
	test( L"ProRes422_HQ_1080i59.94-DF_0ch.mov",  "prores/422_hq", "yuv422p10le" );
	test( L"ProRes422_HQ_1080i59.94-NDF_0ch.mov", "prores/422_hq", "yuv422p10le" );
	test( L"ProRes422_HQ_1080p24_0ch.mov",        "prores/422_hq", "yuv422p10le" );
	test( L"ProRes422_HQ_1080p25_0ch.mov",        "prores/422_hq", "yuv422p10le" );
	test( L"ProRes422_HQ_1080p30_0ch.mov",        "prores/422_hq", "yuv422p10le" );
	test( L"ProRes422_HQ_1080p50_0ch.mov",        "prores/422_hq", "yuv422p10le" );
	test( L"ProRes422_HQ_1080p60_0ch.mov",        "prores/422_hq", "yuv422p10le" );
}

BOOST_AUTO_TEST_CASE( prores_422_lt )
{
	test( L"ProRes422_LT_1080i50_0ch.mov",        "prores/422_lt", "yuv422p10le" );
	test( L"ProRes422_LT_1080i59.94-DF_0ch.mov",  "prores/422_lt", "yuv422p10le" );
	test( L"ProRes422_LT_1080i59.94-NDF_0ch.mov", "prores/422_lt", "yuv422p10le" );
	test( L"ProRes422_LT_1080p24_0ch.mov",        "prores/422_lt", "yuv422p10le" );
	test( L"ProRes422_LT_1080p25_0ch.mov",        "prores/422_lt", "yuv422p10le" );
	test( L"ProRes422_LT_1080p30_0ch.mov",        "prores/422_lt", "yuv422p10le" );
	test( L"ProRes422_LT_1080p50_0ch.mov",        "prores/422_lt", "yuv422p10le" );
	test( L"ProRes422_LT_1080p60_0ch.mov",        "prores/422_lt", "yuv422p10le" );
}

BOOST_AUTO_TEST_CASE( prores_422_proxy )
{
	test( L"ProRes422_Proxy_1080i50_0ch.mov",        "prores/422_proxy", "yuv422p10le" );
	test( L"ProRes422_Proxy_1080i59.94-DF_0ch.mov",  "prores/422_proxy", "yuv422p10le" );
	test( L"ProRes422_Proxy_1080i59.94-NDF_0ch.mov", "prores/422_proxy", "yuv422p10le" );
	test( L"ProRes422_Proxy_1080p24_0ch.mov",        "prores/422_proxy", "yuv422p10le" );
	test( L"ProRes422_Proxy_1080p25_0ch.mov",        "prores/422_proxy", "yuv422p10le" );
	test( L"ProRes422_Proxy_1080p30_0ch.mov",        "prores/422_proxy", "yuv422p10le" );
	test( L"ProRes422_Proxy_1080p50_0ch.mov",        "prores/422_proxy", "yuv422p10le" );
	test( L"ProRes422_Proxy_1080p60_0ch.mov",        "prores/422_proxy", "yuv422p10le" );
}

BOOST_AUTO_TEST_CASE( prores_4444 )
{
	test( L"ProRes4444_1080i50_0ch.mov",        "prores/4444", "yuva444p16le" );
	test( L"ProRes4444_1080i59.94-DF_0ch.mov",  "prores/4444", "yuva444p16le" );
	test( L"ProRes4444_1080i59.94-NDF_0ch.mov", "prores/4444", "yuva444p16le" );
	test( L"ProRes4444_1080p24_0ch.mov",        "prores/4444", "yuva444p16le" );
	test( L"ProRes4444_1080p25_0ch.mov",        "prores/4444", "yuva444p16le" );
	test( L"ProRes4444_1080p30_0ch.mov",        "prores/4444", "yuva444p16le" );
	test( L"ProRes4444_1080p50_0ch.mov",        "prores/4444", "yuva444p16le" );
	test( L"ProRes4444_1080p60_0ch.mov",        "prores/4444", "yuva444p16le" );
}

BOOST_AUTO_TEST_SUITE_END()
