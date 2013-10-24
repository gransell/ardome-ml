#include <boost/test/unit_test.hpp>
#include <openmedialib/ml/utilities.hpp>

using namespace olib::openmedialib::ml;

BOOST_AUTO_TEST_SUITE(deinterlace_filter)

class DataCheckTest
{
    void deinterlace_data_check(const olib::t_string &image_type, int width, int height) const
    {
        image_type_ptr image = image::allocate(image_type, width, height);
        if ( image::coerce< image::image_type_8 >( image ) )
             deinterlace_data_check< image::image_type_8 >( image );
        else if ( image::coerce< image::image_type_16 >( image ) )
            deinterlace_data_check< image::image_type_16 >( image );
    }

	template< typename T >
    void deinterlace_data_check( const image_type_ptr im ) const
    {
		boost::shared_ptr< T > image = image::coerce< T >( im );
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
			typename T::data_type *img = image->data(i);
            int src_pitch = (image->pitch(i) - image->linesize(i)) / sizeof( typename T::data_type );
            
            for (int h = image->height(i) - 1; h >= 0; h--) {
                for (int w = 0; w < ( image->linesize(i) / sizeof( typename T::data_type ) ); w++) {
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

        image->set_field_order(image::top_field_first);
        
        boost::shared_ptr< T > dest = image::coerce< T >( image::deinterlace( image ) );
        
        if (!dest) {
            BOOST_REQUIRE(dest); 
        }
        
        plane_count = dest->plane_count();

        for (int i = 0; i < plane_count; i++) {
			typename T::data_type *dst = dest->data(i);
            int dst_pitch = (dest->pitch(i) - dest->linesize(i)) / sizeof( typename T::data_type );
            
            for (int h = dest->height(i) - 1; h >= 0; h--) {
                for (int w = 0; w < ( dest->linesize(i) / sizeof( typename T::data_type ) ); w++) {
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
        deinterlace_data_check(_CT("yuv444p"), 1920, 1080);
        deinterlace_data_check(_CT("yuv422p"), 1920, 1080);
        deinterlace_data_check(_CT("yuv420p"), 1920, 1080);
        deinterlace_data_check(_CT("yuv411p"), 1920, 1080);
        deinterlace_data_check(_CT("yuv420p10"), 1920, 1080);
        deinterlace_data_check(_CT("yuv420p16"), 1920, 1080);
        deinterlace_data_check(_CT("yuv422p10le"), 1920, 1080);
        deinterlace_data_check(_CT("yuv422p16"), 1920, 1080);
        deinterlace_data_check(_CT("yuv444p16le"), 1920, 1080);
        
        for (int width = 0; width <= 16; width++) {
            for (int height = 0; height <= 16; height++) {
                deinterlace_data_check(_CT("yuv444p"), width, height);
                deinterlace_data_check(_CT("yuv422p"), width, height);
                deinterlace_data_check(_CT("yuv420p"), width, height);
                deinterlace_data_check(_CT("yuv411p"), width, height);
                deinterlace_data_check(_CT("yuv420p10"), width, height);
                deinterlace_data_check(_CT("yuv420p16"), width, height);
                deinterlace_data_check(_CT("yuv422p10le"), width, height);
                deinterlace_data_check(_CT("yuv422p16"), width, height);
                deinterlace_data_check(_CT("yuv444p16le"), width, height);
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
