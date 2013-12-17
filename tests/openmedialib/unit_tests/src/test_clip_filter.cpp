#include <boost/test/unit_test.hpp>
#include <openmedialib/ml/frame.hpp>
#include <openmedialib/plugins/gensys/gensys_plugin.cpp>
#include "utils.hpp"

#define TEST_FILE L"MOV/DVCPro25/DVCPro25_PAL_4ch_16bit.mov"

using namespace olib::openmedialib::ml;
using namespace olib::opencorelib::str_util;


BOOST_AUTO_TEST_CASE(test_clip_filter)
{
    input_type_ptr input = create_input (L"avformat:" MEDIA_REPO_PREFIX_W TEST_FILE);
    
    BOOST_REQUIRE(input);
    BOOST_REQUIRE(input->init());
    
    // Number of audio/video frames
    BOOST_REQUIRE_EQUAL(input->get_frames(), 287);
    
    // Create clip filter
    filter_type_ptr clip_filter = create_filter(L"clip");
    BOOST_REQUIRE(clip_filter);
    
    // Clip filter has zero frames by default
    BOOST_REQUIRE_EQUAL( clip_filter->get_frames(), 0);
    
    // Connect clip filter to an input and sync it
    BOOST_REQUIRE(clip_filter->connect(input));
    clip_filter->sync();
    
    // Configuration
    clip_filter->property("in")  = 0;
    clip_filter->property("out") = -1;
    
    // Clip filter and input should have the same number of frames after connection 
    BOOST_REQUIRE_EQUAL(clip_filter->get_frames(), 287);
    
    // Check different in and out values for full coverage of get_in() and get_out() functions
    clip_filter->property("in") = 300;
    clip_filter->property("out") = 1000;
    BOOST_REQUIRE_EQUAL(clip_filter->get_frames(), 1);
    
    clip_filter->property("in") = -300;
    BOOST_REQUIRE_EQUAL(clip_filter->get_frames(), 287);
    
    clip_filter->property("in") = -1;
    BOOST_REQUIRE_EQUAL(clip_filter->get_frames(), 1);
    
    clip_filter->property("in")  = 0;
    clip_filter->property("out") = 300;
    BOOST_REQUIRE_EQUAL(clip_filter->get_frames(), 287);
    
    clip_filter->property("out") = -300;
    BOOST_REQUIRE_EQUAL(clip_filter->get_frames(), 0);
    
    clip_filter->property("out") = -1;
    BOOST_REQUIRE_EQUAL(clip_filter->get_frames(), 287);
    
    // Special case to verify "in > 0, out > 0, out < in" case
    clip_filter->property("in")  = 5;
    clip_filter->property("out") = 3;
    BOOST_REQUIRE_EQUAL(clip_filter->get_frames(), 2);
    
    // Should always return false since the input will never enforce a packet decode
    BOOST_REQUIRE(!clip_filter->requires_image());
    
    // Basic information
    BOOST_REQUIRE_EQUAL(clip_filter->get_uri(), L"clip");
    
    // Query the current position
    BOOST_REQUIRE_EQUAL(clip_filter->get_position(), 0);
 
    // Seek functionality allows position to overrun the length of the clip
    // whereas the input implementation clamps the position to the length
    clip_filter->seek(5);
    BOOST_REQUIRE_EQUAL(clip_filter->get_position(), 5);
    
    clip_filter->seek(6, true);
    BOOST_REQUIRE_EQUAL(clip_filter->get_position(), 11);
}
