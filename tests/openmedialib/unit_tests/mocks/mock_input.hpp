#pragma once

#include <openmedialib/ml/frame.hpp>
#include <openmedialib/ml/input.hpp>

namespace ml = olib::openmedialib::ml;

class mock_input : public olib::openmedialib::ml::input_type
{
public:
	mock_input ()
	{
		reset ();
	}
	void reset ()
	{
		m_mime_type = L"mock_mime";
		m_get_frames = 0;
		m_seekable = false;
		m_requires_image = false;
		m_frame = olib::openmedialib::ml::frame_type_ptr( new olib::openmedialib::ml::frame_type( ) );
	}	
	virtual const std::wstring get_uri( ) const
	{
		return L"mock_input";
	}
	virtual const std::wstring get_mime_type( ) const
	{
		return m_mime_type;		
	}
	virtual int get_frames() const
	{
		return m_get_frames;
	}
	virtual bool is_seekable() const
	{
		return m_seekable;
	}
	virtual bool requires_image( ) const
	{
		return m_requires_image;
	}	
	virtual void do_fetch( olib::openmedialib::ml::frame_type_ptr & frame)
	{
		frame = m_frame;
	}
	std::wstring m_mime_type;
	int m_get_frames;
	bool m_seekable;
	bool m_requires_image;
	olib::openmedialib::ml::frame_type_ptr m_frame;
};
