#include <boost/test/unit_test.hpp>

#include <openmedialib/ml/utilities.hpp>
#include <openmedialib/ml/store.hpp>
#include <openmedialib/ml/frame.hpp>
#include <openmedialib/ml/input.hpp>
#include <openmedialib/ml/audio.hpp>

#include <fstream>

namespace ml = olib::openmedialib::ml;

static char WAVE_PCM16_1KHz_1Ch [] =
{
	0x52,0x49,0x46,0x46,0x98,0x00,0x00,0x00,0x57,0x41,0x56,0x45,0x66,0x6d,0x74,0x20,
	0x10,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0xe8,0x03,0x00,0x00,0xd0,0x07,0x00,0x00,
	0x02,0x00,0x10,0x00,0x4a,0x55,0x4e,0x4b,0x1c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x64,0x61,0x74,0x61,0x50,0x00,0x00,0x00,
	0x00,0x00,0x02,0x0a,0xc6,0x13,0x0d,0x1d,0x9d,0x25,0x40,0x2d,0xc6,0x33,0x05,0x39,
	0xdd,0x3c,0x35,0x3f,0xff,0x3f,0x35,0x3f,0xdd,0x3c,0x06,0x39,0xc7,0x33,0x41,0x2d,
	0x9e,0x25,0x0f,0x1d,0xc8,0x13,0x04,0x0a,0x01,0x00,0xff,0xf5,0x3b,0xec,0xf4,0xe2,
	0x64,0xda,0xc1,0xd2,0x3b,0xcc,0xfc,0xc6,0x24,0xc3,0xcb,0xc0,0x01,0xc0,0xca,0xc0,
	0x22,0xc3,0xfa,0xc6,0x38,0xcc,0xbe,0xd2,0x60,0xda,0xf0,0xe2,0x37,0xec,0xfb,0xf5
};

static char WAVE_PCM24_1KHz_1Ch [] =
{
	0x52,0x49,0x46,0x46,0xc0,0x00,0x00,0x00,0x57,0x41,0x56,0x45,0x66,0x6d,0x74,0x20,
	0x10,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0xe8,0x03,0x00,0x00,0xb8,0x0b,0x00,0x00,
	0x03,0x00,0x18,0x00,0x4a,0x55,0x4e,0x4b,0x1c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x64,0x61,0x74,0x61,0x78,0x00,0x00,0x00,
	0x00,0x00,0x00,0x80,0xf2,0x02,0x40,0xca,0xc6,0x40,0xfa,0x0d,0x40,0x07,0x9e,0x00,
	0xf8,0x40,0xc0,0xaa,0xc6,0x80,0x02,0x06,0x00,0xec,0xdd,0xc0,0x2d,0x36,0x00,0x00,
	0x00,0x80,0x6a,0x36,0x40,0x64,0xde,0x00,0xb3,0x06,0x00,0x8f,0xc7,0x00,0x0b,0x42,
	0x80,0x41,0x9f,0x80,0x54,0x0f,0xe0,0x3b,0xc8,0x50,0x72,0x04,0x9e,0x84,0x01,0x50,
	0x8d,0xfe,0x60,0xa7,0x3a,0x00,0x60,0xf3,0x40,0x33,0x63,0xc0,0x1a,0xc0,0xc0,0x39,
	0x3a,0x00,0xae,0xfa,0x00,0x8c,0x22,0x00,0x0f,0xca,0x00,0x00,0x00,0xc0,0x58,0xc9,
	0xc0,0x23,0x21,0xc0,0x9c,0xf8,0x80,0x8c,0x37,0x40,0xe2,0xbc,0x00,0x84,0x5f,0x40,
	0x51,0xef,0xa0,0x52,0x36,0xd0,0x0d,0xfa
};

static char WAVE_PCM32_1KHz_1Ch [] =
{
	0x52,0x49,0x46,0x46,0xe8,0x00,0x00,0x00,0x57,0x41,0x56,0x45,0x66,0x6d,0x74,0x20,
	0x10,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0xe8,0x03,0x00,0x00,0xa0,0x0f,0x00,0x00,
	0x04,0x00,0x20,0x00,0x4a,0x55,0x4e,0x4b,0x1c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x64,0x61,0x74,0x61,0xa0,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x80,0xf2,0x02,0x0a,0x40,0xca,0xc6,0x13,0x40,0xfa,0x0d,0x1d,
	0x40,0x07,0x9e,0x25,0x00,0xf8,0x40,0x2d,0xc0,0xaa,0xc6,0x33,0x80,0x02,0x06,0x39,
	0x00,0xec,0xdd,0x3c,0xc0,0x2d,0x36,0x3f,0x00,0x00,0x00,0x40,0x80,0x6a,0x36,0x3f,
	0x40,0x64,0xde,0x3c,0x00,0xb3,0x06,0x39,0x00,0x8f,0xc7,0x33,0x00,0x0b,0x42,0x2d,
	0x80,0x41,0x9f,0x25,0x80,0x54,0x0f,0x1d,0xe0,0x3b,0xc8,0x13,0x50,0x72,0x04,0x0a,
	0x9e,0x84,0x01,0x00,0x50,0x8d,0xfe,0xf5,0x60,0xa7,0x3a,0xec,0x00,0x60,0xf3,0xe2,
	0x40,0x33,0x63,0xda,0xc0,0x1a,0xc0,0xd2,0xc0,0x39,0x3a,0xcc,0x00,0xae,0xfa,0xc6,
	0x00,0x8c,0x22,0xc3,0x00,0x0f,0xca,0xc0,0x00,0x00,0x00,0xc0,0xc0,0x58,0xc9,0xc0,
	0xc0,0x23,0x21,0xc3,0xc0,0x9c,0xf8,0xc6,0x80,0x8c,0x37,0xcc,0x40,0xe2,0xbc,0xd2,
	0x00,0x84,0x5f,0xda,0x40,0x51,0xef,0xe2,0xa0,0x52,0x36,0xec,0xd0,0x0d,0xfa,0xf5
};

static char WAVE_IEEE_FLOAT_1KHz_1Ch [] =
{
	0x52,0x49,0x46,0x46,0xe8,0x00,0x00,0x00,0x57,0x41,0x56,0x45,0x66,0x6d,0x74,0x20,
	0x10,0x00,0x00,0x00,0x03,0x00,0x01,0x00,0xe8,0x03,0x00,0x00,0xa0,0x0f,0x00,0x00,
	0x04,0x00,0x20,0x00,0x4a,0x55,0x4e,0x4b,0x1c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x64,0x61,0x74,0x61,0xa0,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x28,0x2f,0xa0,0x3d,0x52,0x36,0x1e,0x3e,0xd2,0x6f,0x68,0x3e,
	0x1d,0x78,0x96,0x3e,0xe0,0x03,0xb5,0x3e,0xab,0x1a,0xcf,0x3e,0x0a,0x18,0xe4,0x3e,
	0xb0,0x77,0xf3,0x3e,0xb7,0xd8,0xfc,0x3e,0x00,0x00,0x00,0x3f,0xaa,0xd9,0xfc,0x3e,
	0x91,0x79,0xf3,0x3e,0xcc,0x1a,0xe4,0x3e,0x3c,0x1e,0xcf,0x3e,0x2c,0x08,0xb5,0x3e,
	0x06,0x7d,0x96,0x3e,0xa4,0x7a,0x68,0x3e,0xdf,0x41,0x1e,0x3e,0x25,0x47,0xa0,0x3d,
	0x04,0x4f,0x42,0x38,0x2b,0x17,0xa0,0xbd,0xc5,0x2a,0x1e,0xbe,0x00,0x65,0x68,0xbe,
	0x33,0x73,0x96,0xbe,0x95,0xff,0xb4,0xbe,0x19,0x17,0xcf,0xbe,0x48,0x15,0xe4,0xbe,
	0xd0,0x75,0xf3,0xbe,0xc4,0xd7,0xfc,0xbe,0x00,0x00,0x00,0xbf,0x9d,0xda,0xfc,0xbe,
	0x71,0x7b,0xf3,0xbe,0x8d,0x1d,0xe4,0xbe,0xce,0x21,0xcf,0xbe,0x77,0x0c,0xb5,0xbe,
	0xf0,0x81,0x96,0xbe,0x76,0x85,0x68,0xbe,0x6b,0x4d,0x1e,0xbe,0x23,0x5f,0xa0,0xbd
};

static char WAVE_PCM16_1KHz_1Ch_39Samples [] =
{
	0x52,0x49,0x46,0x46,0x96,0x00,0x00,0x00,0x57,0x41,0x56,0x45,0x66,0x6d,0x74,0x20,
	0x10,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0xe8,0x03,0x00,0x00,0xd0,0x07,0x00,0x00,
	0x02,0x00,0x10,0x00,0x4a,0x55,0x4e,0x4b,0x1c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x64,0x61,0x74,0x61,0x4e,0x00,0x00,0x00,
	0x00,0x00,0x02,0x0a,0xc6,0x13,0x0d,0x1d,0x9d,0x25,0x40,0x2d,0xc6,0x33,0x05,0x39,
	0xdd,0x3c,0x35,0x3f,0xff,0x3f,0x35,0x3f,0xdd,0x3c,0x06,0x39,0xc7,0x33,0x41,0x2d,
	0x9e,0x25,0x0f,0x1d,0xc8,0x13,0x04,0x0a,0x01,0x00,0xff,0xf5,0x3b,0xec,0xf4,0xe2,
	0x64,0xda,0xc1,0xd2,0x3b,0xcc,0xfc,0xc6,0x24,0xc3,0xcb,0xc0,0x01,0xc0,0xca,0xc0,
	0x22,0xc3,0xfa,0xc6,0x38,0xcc,0xbe,0xd2,0x60,0xda,0xf0,0xe2,0x37,0xec
};

static char WAVE_PCM16_1KHz_1Ch_41Samples [] =
{
	0x52,0x49,0x46,0x46,0x9a,0x00,0x00,0x00,0x57,0x41,0x56,0x45,0x66,0x6d,0x74,0x20,
	0x10,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0xe8,0x03,0x00,0x00,0xd0,0x07,0x00,0x00,
	0x02,0x00,0x10,0x00,0x4a,0x55,0x4e,0x4b,0x1c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x64,0x61,0x74,0x61,0x52,0x00,0x00,0x00,
	0x00,0x00,0x02,0x0a,0xc6,0x13,0x0d,0x1d,0x9d,0x25,0x40,0x2d,0xc6,0x33,0x05,0x39,
	0xdd,0x3c,0x35,0x3f,0xff,0x3f,0x35,0x3f,0xdd,0x3c,0x06,0x39,0xc7,0x33,0x41,0x2d,
	0x9e,0x25,0x0f,0x1d,0xc8,0x13,0x04,0x0a,0x01,0x00,0xff,0xf5,0x3b,0xec,0xf4,0xe2,
	0x64,0xda,0xc1,0xd2,0x3b,0xcc,0xfc,0xc6,0x24,0xc3,0xcb,0xc0,0x01,0xc0,0xca,0xc0,
	0x22,0xc3,0xfa,0xc6,0x38,0xcc,0xbe,0xd2,0x60,0xda,0xf0,0xe2,0x37,0xec,0xfb,0xf5,0xfb,0xf5
};

BOOST_AUTO_TEST_SUITE( wav_input )

class TestEnv
{
public:
	void Init (const char* filepath, char* data, int length)
	{
		m_file.open(filepath, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
		m_file.write (data, length);
		m_file.flush ();
	}
	std::ofstream m_file;
};

BOOST_FIXTURE_TEST_CASE( Test_WAVE_PCM16_1KHz_1Ch, TestEnv )
{
	Init ("WAVE_PCM16_1KHz_1Ch.wav", WAVE_PCM16_1KHz_1Ch, sizeof (WAVE_PCM16_1KHz_1Ch));

	ml::input_type_ptr wav_input = ml::create_input( "WAVE_PCM16_1KHz_1Ch.wav");
	BOOST_REQUIRE (wav_input != NULL);
	
	ml::frame_type_ptr frame = wav_input->fetch ();
	BOOST_REQUIRE (frame != NULL);
	
	BOOST_CHECK_EQUAL (frame->has_audio(), true);
	ml::audio_type_ptr audio = frame->get_audio( );
	
	BOOST_REQUIRE (audio != NULL);
	
	BOOST_CHECK_EQUAL (audio->id(), ml::audio::pcm16_id);
	BOOST_CHECK_EQUAL (audio->frequency(), 1000);
	BOOST_CHECK_EQUAL (audio->channels(), 1);
	BOOST_CHECK_EQUAL (audio->samples(), 40);
}

BOOST_FIXTURE_TEST_CASE( Test_WAVE_PCM24_1KHz_1Ch, TestEnv )
{
	Init ("WAVE_PCM24_1KHz_1Ch.wav", WAVE_PCM24_1KHz_1Ch, sizeof (WAVE_PCM24_1KHz_1Ch));

	ml::input_type_ptr wav_input = ml::create_input( "WAVE_PCM24_1KHz_1Ch.wav");
	BOOST_REQUIRE (wav_input != NULL);
	
	ml::frame_type_ptr frame = wav_input->fetch ();
	BOOST_REQUIRE (frame != NULL);
	
	BOOST_CHECK_EQUAL (frame->has_audio(), true);
	ml::audio_type_ptr audio = frame->get_audio( );
	
	BOOST_REQUIRE (audio != NULL);
	
	BOOST_CHECK_EQUAL (audio->id(), ml::audio::pcm24_id);
	BOOST_CHECK_EQUAL (audio->frequency(), 1000);
	BOOST_CHECK_EQUAL (audio->channels(), 1);
	BOOST_CHECK_EQUAL (audio->samples(), 40);
}

BOOST_FIXTURE_TEST_CASE( Test_WAVE_PCM32_1KHz_1Ch, TestEnv )
{
	Init ("WAVE_PCM32_1KHz_1Ch.wav", WAVE_PCM32_1KHz_1Ch, sizeof (WAVE_PCM32_1KHz_1Ch));

	ml::input_type_ptr wav_input = ml::create_input( "WAVE_PCM32_1KHz_1Ch.wav");
	BOOST_REQUIRE (wav_input != NULL);
	
	ml::frame_type_ptr frame = wav_input->fetch ();
	BOOST_REQUIRE (frame != NULL);
	
	BOOST_CHECK_EQUAL (frame->has_audio(), true);
	ml::audio_type_ptr audio = frame->get_audio( );
	
	BOOST_REQUIRE (audio != NULL);
	
	BOOST_CHECK_EQUAL (audio->id(), ml::audio::pcm32_id);
	BOOST_CHECK_EQUAL (audio->frequency(), 1000);
	BOOST_CHECK_EQUAL (audio->channels(), 1);
	BOOST_CHECK_EQUAL (audio->samples(), 40);
}

BOOST_FIXTURE_TEST_CASE( Test_WAVE_IEEE_FLOAT_1KHz_1Ch, TestEnv )
{
	Init ("WAVE_IEEE_FLOAT_1KHz_1Ch.wav", WAVE_IEEE_FLOAT_1KHz_1Ch, sizeof (WAVE_IEEE_FLOAT_1KHz_1Ch));	

	ml::input_type_ptr wav_input = ml::create_input( "WAVE_IEEE_FLOAT_1KHz_1Ch.wav");
	BOOST_REQUIRE (wav_input != NULL);
	
	ml::frame_type_ptr frame = wav_input->fetch ();
	BOOST_REQUIRE (frame != NULL);
	
	BOOST_CHECK_EQUAL (frame->has_audio(), true);
	ml::audio_type_ptr audio = frame->get_audio( );
	
	BOOST_REQUIRE (audio != NULL);
	
	BOOST_CHECK_EQUAL (audio->id(), ml::audio::float_id);
	BOOST_CHECK_EQUAL (audio->frequency(), 1000);
	BOOST_CHECK_EQUAL (audio->channels(), 1);
	BOOST_CHECK_EQUAL (audio->samples(), 40);	
}

BOOST_FIXTURE_TEST_CASE( Test_WAVE_PCM16_1KHz_32Ch, TestEnv )
{
	char data [sizeof (WAVE_PCM16_1KHz_1Ch) - 40 * 2] = {};
	memcpy (data, WAVE_PCM16_1KHz_1Ch, sizeof (WAVE_PCM16_1KHz_1Ch) - 40 * 2);
	data [22] = 32;	// ftm.Number_of_channels
	
	Init ("WAVE_PCM16_1KHz_32Ch.wav", data, sizeof (data));

	for (int s = 0; s < 40; ++ s)
	{
		for (int c = 0; c < 32; ++ c)
		{
			char v[2] = {};
			m_file.write (v, 2);
		}		
	}

	m_file.flush();

	ml::input_type_ptr wav_input = ml::create_input( "WAVE_PCM16_1KHz_32Ch.wav");
	BOOST_REQUIRE (wav_input != NULL);
	
	ml::frame_type_ptr frame = wav_input->fetch ();
	BOOST_REQUIRE (frame != NULL);
	
	BOOST_CHECK_EQUAL (frame->has_audio(), true);
	ml::audio_type_ptr audio = frame->get_audio( );
	
	BOOST_REQUIRE (audio != NULL);
	
	BOOST_CHECK_EQUAL (audio->id(), ml::audio::pcm16_id);
	BOOST_CHECK_EQUAL (audio->frequency(), 1000);
	BOOST_CHECK_EQUAL (audio->channels(), 32);
	BOOST_CHECK_EQUAL (audio->samples(), 40);
}

BOOST_FIXTURE_TEST_CASE( Test_WAVE_PCM24_1KHz_32Ch, TestEnv )
{
	char data [sizeof (WAVE_PCM24_1KHz_1Ch) - 40 * 3] = {};
	memcpy (data, WAVE_PCM24_1KHz_1Ch, sizeof (WAVE_PCM24_1KHz_1Ch) - 40 * 3);
	data [22] = 32;// ftm.Number_of_channels

	Init ("WAVE_PCM24_1KHz_32Ch.wav", data, sizeof (data));
	
	for (int s = 0; s < 40; ++ s)
	{
		for (int c = 0; c < 32; ++ c)
		{
			char v[3] = {};
			m_file.write (v, 3);
		}		
	}

	m_file.flush();
	
	ml::input_type_ptr wav_input = ml::create_input( "WAVE_PCM24_1KHz_32Ch.wav");
	BOOST_REQUIRE (wav_input != NULL);
	
	ml::frame_type_ptr frame = wav_input->fetch ();
	BOOST_REQUIRE (frame != NULL);
	
	BOOST_CHECK_EQUAL (frame->has_audio(), true);
	ml::audio_type_ptr audio = frame->get_audio( );
	
	BOOST_REQUIRE (audio != NULL);
	
	BOOST_CHECK_EQUAL (audio->id(), ml::audio::pcm24_id);
	BOOST_CHECK_EQUAL (audio->frequency(), 1000);
	BOOST_CHECK_EQUAL (audio->channels(), 32);
	BOOST_CHECK_EQUAL (audio->samples(), 40);
}

BOOST_FIXTURE_TEST_CASE( Test_WAVE_PCM32_1KHz_32Ch, TestEnv )
{
	char data [sizeof (WAVE_PCM32_1KHz_1Ch) - 40 * 4] = {};
	memcpy (data, WAVE_PCM32_1KHz_1Ch, sizeof (WAVE_PCM32_1KHz_1Ch) - 40 * 4);
	data [22] = 32;// ftm.Number_of_channels

	Init ("WAVE_PCM32_1KHz_32Ch.wav", data, sizeof (data));

	for (int s = 0; s < 40; ++ s)
	{
		for (int c = 0; c < 32; ++ c)
		{
			char v[4] = {};
			m_file.write (v, 4);
		}		
	}

	m_file.flush();

	ml::input_type_ptr wav_input = ml::create_input( "WAVE_PCM32_1KHz_32Ch.wav");
	BOOST_REQUIRE (wav_input != NULL);
	
	ml::frame_type_ptr frame = wav_input->fetch ();
	BOOST_REQUIRE (frame != NULL);
	
	BOOST_CHECK_EQUAL (frame->has_audio(), true);
	ml::audio_type_ptr audio = frame->get_audio( );
	
	BOOST_REQUIRE (audio != NULL);
	
	BOOST_CHECK_EQUAL (audio->id(), ml::audio::pcm32_id);
	BOOST_CHECK_EQUAL (audio->frequency(), 1000);
	BOOST_CHECK_EQUAL (audio->channels(), 32);
	BOOST_CHECK_EQUAL (audio->samples(), 40);
}

BOOST_FIXTURE_TEST_CASE( Test_WAVE_IEEE_FLOAT_1KHz_32Ch, TestEnv )
{
	char data [sizeof (WAVE_IEEE_FLOAT_1KHz_1Ch) - 40 * 4] = {};
	memcpy (data, WAVE_IEEE_FLOAT_1KHz_1Ch, sizeof (WAVE_IEEE_FLOAT_1KHz_1Ch) - 40 * 4);
	data [22] = 32;	// ftm.Number_of_channels
	Init ("WAVE_IEEE_FLOAT_1KHz_32Ch.wav", data, sizeof (data));

	for (int s = 0; s < 40; ++ s)
	{
		for (int c = 0; c < 32; ++ c)
		{
			char v[4] = {};
			m_file.write (v, 4);
		}		
	}

	m_file.flush();

	ml::input_type_ptr wav_input = ml::create_input( "WAVE_IEEE_FLOAT_1KHz_32Ch.wav");
	BOOST_REQUIRE (wav_input != NULL);
	
	ml::frame_type_ptr frame = wav_input->fetch ();
	BOOST_REQUIRE (frame != NULL);
	
	BOOST_CHECK_EQUAL (frame->has_audio(), true);
	ml::audio_type_ptr audio = frame->get_audio( );
	
	BOOST_REQUIRE (audio != NULL);
	
	BOOST_CHECK_EQUAL (audio->id(), ml::audio::float_id);
	BOOST_CHECK_EQUAL (audio->frequency(), 1000);
	BOOST_CHECK_EQUAL (audio->channels(), 32);
	BOOST_CHECK_EQUAL (audio->samples(), 40);	
}

BOOST_FIXTURE_TEST_CASE( Test_NumberOfFramesIsOneWhenSamplesFillFrame, TestEnv )
{
	Init ("WAVE_PCM16_1KHz_1Ch.wav", WAVE_PCM16_1KHz_1Ch, sizeof (WAVE_PCM16_1KHz_1Ch));

	ml::input_type_ptr wav_input = ml::create_input( "WAVE_PCM16_1KHz_1Ch.wav");
	BOOST_REQUIRE (wav_input != NULL);
	
	ml::frame_type_ptr frame = wav_input->fetch ();
	BOOST_REQUIRE (frame != NULL);
	BOOST_CHECK_EQUAL (wav_input->get_frames (), 1);	
}

BOOST_FIXTURE_TEST_CASE( Test_NumberOfFramesIsOneWhenTooLessSamplesToFillFrame, TestEnv )
{
	Init ("WAVE_PCM16_1KHz_1Ch_39Samples.wav", WAVE_PCM16_1KHz_1Ch_39Samples, sizeof (WAVE_PCM16_1KHz_1Ch_39Samples));

	ml::input_type_ptr wav_input = ml::create_input( "WAVE_PCM16_1KHz_1Ch_39Samples.wav");
	BOOST_REQUIRE (wav_input != NULL);
	
	ml::frame_type_ptr frame = wav_input->fetch ();
	BOOST_REQUIRE (frame != NULL);
	BOOST_CHECK_EQUAL (wav_input->get_frames (), 1);	
}

BOOST_FIXTURE_TEST_CASE( Test_NumberOfFramesIsTwoWhenOneMoreSamplesThanFrameBoundary, TestEnv )
{
	Init ("WAVE_PCM16_1KHz_1Ch_41Samples.wav", WAVE_PCM16_1KHz_1Ch_41Samples, sizeof (WAVE_PCM16_1KHz_1Ch_41Samples));

	ml::input_type_ptr wav_input = ml::create_input( "WAVE_PCM16_1KHz_1Ch_41Samples.wav");
	BOOST_REQUIRE (wav_input != NULL);
	
	ml::frame_type_ptr frame = wav_input->fetch ();
	BOOST_REQUIRE (frame != NULL);
	BOOST_CHECK_EQUAL (wav_input->get_frames (), 2);
}

BOOST_AUTO_TEST_SUITE_END()
