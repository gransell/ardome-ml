// AMF Decklink Media Plugin
//
// Copyright (C) 2008 Ardendo


#include <opencorelib/cl/core.hpp>
#include <opencorelib/cl/log_defines.hpp>
#include <opencorelib/cl/assert_defines.hpp>
#include <opencorelib/cl/enforce_defines.hpp>

#include <openmedialib/ml/input.hpp>

namespace cl = olib::opencorelib;
namespace ml = olib::openmedialib::ml;
namespace pl = olib::openpluginlib;

namespace amf { namespace openmedialib {
	
	class ML_PLUGIN_DECLSPEC input_decklink : public ml::input_type
	{
	public:
		// Constructor and destructor
		input_decklink( const pl::wstring &filename ) : uri_( filename )
		{
		}
		
		virtual ~input_decklink( ) {}
		
		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }
		
		// Basic information
		virtual const pl::wstring get_uri( ) const { return uri_; }
		virtual const pl::wstring get_mime_type( ) const { return L""; }
		
		// Audio/Visual
		virtual int get_frames( ) const
		{
			return std::numeric_limits< int >::max();
		}
		virtual bool is_seekable( ) const
		{
			return false;
		}
		
		// Visual
		virtual int get_video_streams( ) const
		{
			return 1;
		}
		
		// Audio
		virtual int get_audio_streams( ) const
		{
			return 0;
		}
		
		virtual int get_audio_channels_in_stream( int stream_index ) const
		{
			return 0;
		}
		
	protected:
		
		virtual bool initialize( )
		{
			return true;
		}
		
		// Fetch method
		void do_fetch( ml::frame_type_ptr &result )
		{
			
		}
		
		virtual bool complete( ) const
		{
			return true;
		}
		
	private:
		const pl::wstring uri_;
	};
	
	
	ml::input_type_ptr ML_PLUGIN_DECLSPEC create_input_decklink( const pl::wstring &filename )
	{
		return ml::input_type_ptr( new input_decklink( filename ) );
	}
	
} }



