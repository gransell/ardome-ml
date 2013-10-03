#pragma once

#include <openmedialib/ml/frame.hpp>

namespace olib { namespace openmedialib { namespace ml { namespace unittest {

class mock_frame : public frame_type
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
		image_.reset();
	}

	virtual olib::openmedialib::ml::image_type_ptr get_image()
	{
		m_state_num_get_image_calls++;
		return frame_type::get_image();
	}

	virtual void set_position( int position )
	{
		m_ret_get_position = position;
	}

	virtual int get_position() const
	{
		return m_ret_get_position;
	}

	virtual frame_type_ptr shallow( ) const
	{
		mock_frame *copy = new mock_frame(this);
		return frame_type_ptr( copy );
	}

	int m_ret_get_position;
	int m_state_num_get_image_calls;
protected:
	mock_frame( const mock_frame *other ) : frame_type (other)
	{
		m_ret_get_position = other->m_ret_get_position;
		m_state_num_get_image_calls = other->m_state_num_get_image_calls;
	}
};

} } } }
