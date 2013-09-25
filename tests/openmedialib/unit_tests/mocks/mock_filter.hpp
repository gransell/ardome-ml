#pragma once

#include <openmedialib/ml/frame.hpp>
#include <openmedialib/ml/filter.hpp>

namespace ml = olib::openmedialib::ml;

class mock_filter : public olib::openmedialib::ml::filter_type
{
public:
	mock_filter ()
	{
		reset ();
	}
	void reset ()
	{
		m_requires_image = false;
		m_frame = olib::openmedialib::ml::frame_type_ptr( new olib::openmedialib::ml::frame_type( ) );		
	}
	virtual const std::wstring get_uri( ) const
	{
		return L"mock_filter";
	}
	virtual bool requires_image( ) const
	{
		return m_requires_image;
	}
	virtual void do_fetch( olib::openmedialib::ml::frame_type_ptr & frame)
	{
		frame = m_frame;		
	}
	
	bool m_requires_image;
	olib::openmedialib::ml::frame_type_ptr m_frame;
};
