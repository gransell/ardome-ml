#include <boost/test/unit_test.hpp>
#include <openmedialib/ml/utilities.hpp>

using namespace olib::openimagelib::il;

BOOST_AUTO_TEST_SUITE(deinterlace_filter)

class DataCheckTest
{
    void deinterlace_data_check(const std::wstring &image_type, int width, int height) const
    {
        image_type_ptr image = allocate(image_type, width, height);
        
        if (!image) {
            BOOST_REQUIRE(image);
        }
        
        int plane_count = image->plane_count();
        
        std::vector<int> data_odd;
        std::vector<int> data_even;
        
        int number = 0;
        
        for (int i = 0; i < plane_count; i++) {
            data_odd.push_back(2 * number);
            data_even.push_back(2 * number + 1);
            
            number++;
        }
        
        for (int i = 0; i < plane_count; i++) {
            unsigned char *img      = image->data(i);
            int           src_pitch = image->pitch(i) - image->linesize(i);
            
            for (int h = image->height(i) - 1; h >= 0; h--) {
                for (int w = 0; w < image->linesize(i); w++) {
                    if (h % 2 == 0) {
                        *img = data_even[i];
                    } else {
                        *img = data_odd[i];
                    }
                    img++;
                }
                img += src_pitch;
            }
        }

        image->set_field_order(top_field_first);
        
        image_type_ptr dest = deinterlace(image);
        
        if (!dest) {
            BOOST_REQUIRE(dest); 
        }
        
        plane_count = dest->plane_count();

        for (int i = 0; i < plane_count; i++) {
            unsigned char *dst      = dest->data(i);
            int           dst_pitch = dest->pitch(i) - dest->linesize(i);
            
            for (int h = dest->height(i) - 1; h >= 0; h--) {
                for (int w = 0; w < dest->linesize(i); w++) {
                    if (h == 0) {
                        if (*dst != data_even[i]) {
                            BOOST_REQUIRE_EQUAL(*dst, data_even[i]);
                        }
                    } else {
                        if (*dst != (data_odd[i] + data_even[i])/2) {
                            BOOST_REQUIRE_EQUAL(*dst, (data_odd[i] + data_even[i])/2);
                        }
                    }
                    dst++;
                }
                dst += dst_pitch;
            }
        }
    }
    
public:
    void verify() const
    {
        deinterlace_data_check(L"yuv444p", 1920, 1080);
        deinterlace_data_check(L"yuv422p", 1920, 1080);
        deinterlace_data_check(L"yuv420p", 1920, 1080);
        deinterlace_data_check(L"yuv411p", 1920, 1080);
        
        for (int width = 0; width <= 16; width++) {
            for (int height = 0; height <= 16; height++) {
                deinterlace_data_check(L"yuv444p", width, height);
                deinterlace_data_check(L"yuv422p", width, height);
                deinterlace_data_check(L"yuv420p", width, height);
                deinterlace_data_check(L"yuv411p", width, height);
            }
        }
        
    };
};

BOOST_AUTO_TEST_CASE(data_check)
{
    DataCheckTest deinterlace_check;
    deinterlace_check.verify();
    
}
    
BOOST_AUTO_TEST_SUITE_END()
