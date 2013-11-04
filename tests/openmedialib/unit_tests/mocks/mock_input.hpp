#pragma once
#include "mock_frame.hpp"
#include <boost/test/unit_test.hpp>
#include <openmedialib/ml/input.hpp>

namespace olib { namespace openmedialib { namespace ml { namespace unittest {

class mock_input : public input_type
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
		m_get_position = 0;
		m_num_fetch_calls = 0;
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
	virtual void seek( const int pos, const bool relative )
	{
		//Relative seek not implemented
		ARENFORCE( !relative );
		m_get_position = pos;
	}
	virtual void do_fetch( frame_type_ptr & frame )
	{
		++m_num_fetch_calls;
		m_frame.m_ret_get_position = m_get_position;
		frame = m_frame.shallow();
	}
	std::wstring m_mime_type;
	int m_get_frames;
	bool m_seekable;
	bool m_requires_image;
	int m_get_position;
	int m_num_fetch_calls;
	mock_frame m_frame;
};

} } } }
