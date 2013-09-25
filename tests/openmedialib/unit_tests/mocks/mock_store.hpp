#pragma once

#include <openmedialib/ml/store.hpp>
#include <openmedialib/ml/frame.hpp>

namespace ml = olib::openmedialib::ml;
namespace pcos = olib::openpluginlib::pcos;

class mock_store : public olib::openmedialib::ml::store_type
{
public:
	mock_store ()
	{
		reset ();
	}
	void reset ()
	{
		m_push_return = true;
		m_create_count = 0;
		m_complete = false;
	}
	virtual bool push( olib::openmedialib::ml::frame_type_ptr frame)
	{
		m_frame = frame;
		return m_push_return;
	}
	virtual void complete( )
	{
		m_complete = true;
	}
	bool m_push_return;
	int m_create_count;
	olib::openmedialib::ml::frame_type_ptr m_frame;
	bool m_complete;
};
