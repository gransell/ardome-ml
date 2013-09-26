#include <openmedialib/ml/frame.hpp>

class mock_frame : public olib::openmedialib::ml::frame_type
{
public:
	mock_frame()
	{
		reset();
	}

	void reset()
	{
		m_ret_get_position = 0;

		m_state_num_get_image_calls = 0;
	}

	virtual olib::openimagelib::il::image_type_ptr get_image()
	{
		m_state_num_get_image_calls++;
		return frame_type::get_image();
	}

	virtual int get_position() const
	{
		return m_ret_get_position;
	}

	virtual olib::openmedialib::ml::frame_type_ptr shallow( ) const
	{
		mock_frame *copy = new mock_frame();
		copy->m_ret_get_position = this->m_ret_get_position;
		copy->m_state_num_get_image_calls = this->m_state_num_get_image_calls;
		return olib::openmedialib::ml::frame_type_ptr( copy );
	}

	int m_ret_get_position;
	int m_state_num_get_image_calls;
};
