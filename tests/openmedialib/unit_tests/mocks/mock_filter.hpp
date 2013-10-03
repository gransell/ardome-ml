#pragma once

#include <openmedialib/ml/frame.hpp>
#include <openmedialib/ml/filter.hpp>

namespace olib { namespace openmedialib { namespace ml { namespace unittest {

class mock_filter : public filter_type
{
public:
	mock_filter ()
	{
		reset ();
	}
	void reset ()
	{
		m_requires_image = false;
		m_frame = frame_type_ptr( new frame_type( ) );		
	}
	virtual const std::wstring get_uri( ) const
	{
		return L"mock_filter";
	}
	virtual bool requires_image( ) const
	{
		return m_requires_image;
	}
	virtual void do_fetch( frame_type_ptr & frame)
	{
		frame = m_frame;		
	}
	
	bool m_requires_image;
	frame_type_ptr m_frame;
};

} } } }
