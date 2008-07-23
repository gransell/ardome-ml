
// mlt - A mlt plugin to ml.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#define WIN32_LEAN_AND_MEAN
#include <mlt++/Mlt.h>

#include <boost/thread/recursive_mutex.hpp>

#include <openmedialib/ml/openmedialib_plugin.hpp>

namespace opl = olib::openpluginlib;
namespace plugin = olib::openmedialib::ml;
namespace il = olib::openimagelib::il;

namespace olib { namespace openmedialib { namespace ml { 

class ML_PLUGIN_DECLSPEC mlt_frame : public frame_type
{
	typedef il::image< unsigned char, il::r8g8b8 > r8g8b8_image_type;

	public:
		mlt_frame( Mlt::Frame *frame, int pts, int process_flags )
			: frame_( frame )
			, pf_( process_flags )
		{
			set_position( pts );
			set_pts( double( pts ) );
			set_duration( 1.0 / frame->get_double( "fps" ) );
			set_sar( 1, 1 );
			width_ = frame_->get_int( "width" );
			height_ = frame_->get_int( "height" );
			sar_ = frame_->get_double( "aspect_ratio" );
		}

		~mlt_frame( )
		{
			delete frame_;
		}

		void render_image( )
		{
			if ( frame_ && frame_->get_int( "test_image" ) == 0 )
			{
				typedef il::image< unsigned char, il::yuv422 > yuv422_image_type;
				frame_->set( "deinterlace_method", "weave" );
				frame_->set( "consumer_deinterlace", 1 );
				frame_->set( "rescale.interp", "hyper" );
				frame_->set( "resize", 1 );
				frame_->set( "consumer_aspect_ratio", sar_ );
				int w = width_;
				int h = height_;
				mlt_image_format format = mlt_image_yuv422;
				uint8_t *image = frame_->get_image( format, w, h );
				image_ = image_type_ptr( new image_type( yuv422_image_type( w, h, 1 ) ) );
				w *= 2;
				int stride = image_->pitch( );
				uint8_t *ptr = image_->data( );
				image_->set_position( get_position( ) );
				while ( h -- )
				{
					memcpy( ptr, image, w );
					ptr += stride;
					image += w;
				}
			}
		}

		void render_audio( )
		{
			if ( frame_ )
			{
				typedef audio< unsigned char, pcm16 > pcm16_audio_type;
				mlt_audio_format format = mlt_audio_pcm;
				int frequency = 44100;
				int channels = 2;
				float fps = float( frame_->get_double( "fps" ) );
				int samples = mlt_sample_calculator( fps, frequency, int64_t( get_pts( ) ) );
				short *audio = frame_->get_audio( format, frequency, channels, samples );
				audio_ = audio_type_ptr( new audio_type( pcm16_audio_type( frequency, channels, samples ) ) );
				audio_->set_position( get_position( ) );
				memcpy( audio_->data( ), audio, audio_->size( ) );
			}
		}

		virtual image_type_ptr get_image( ) 
		{ 
			if ( pf_ & ml::process_image && image_ == 0 )
				render_image( );
			return image_; 
		}

		virtual void set_image( image_type_ptr image )
		{
			image_ = image;
		}

		virtual audio_type_ptr get_audio( ) 
		{
			if ( pf_ & ml::process_audio && audio_ == 0 )
				render_audio( );
			return audio_;
		}

		virtual double aspect_ratio( )
		{
			return sar_ * width_ / height_;
		}

	private:
		Mlt::Frame *frame_;
		int pf_;
		int sar_num_, sar_den_;
		double duration_;
		image_type_ptr image_;
		audio_type_ptr audio_;
		int width_;
		int height_;
		double sar_;
};

class ML_PLUGIN_DECLSPEC mlt_input : public input_type
{
	public:
		// Constructor and destructor
		explicit mlt_input( const opl::wstring media )
			: input_type( ) 
			, media_( media )
			, producer_( 0 )
			, has_video_( false )
			, has_audio_( false )
		{
		}

		virtual ~mlt_input( ) 
		{ 
			delete producer_;
		}

		// Basic information
		virtual const opl::wstring get_uri( ) const 
		{ 
			return media_;
		}

		virtual const opl::wstring get_mime_type( ) const 
		{ 
			return L""; 
		}

		virtual bool has_video( ) const 
		{ 
			return has_video_; 
		}

		virtual bool has_audio( ) const 
		{ 
			return has_audio_; 
		}

		// Audio/Visual
		virtual int get_frames( ) const 
		{
			return producer_ && producer_->is_valid( ) ? producer_->get_length( ) : 0; 
		}

		virtual bool is_seekable( ) const 
		{ 
			return true; 
		}

		// Visual
		virtual void get_fps( int &num, int &den ) const 
		{ 
			num = 25; den = 1; 
		}

		virtual void get_sar( int &num, int &den ) const 
		{ 
			num = 1; den = 1; 
		}

		virtual int get_video_streams( ) const 
		{ 
			return 1; 
		}

		virtual int get_width( ) const 
		{ 
			return 720; 
		}

		virtual int get_height( ) const 
		{ 
			return 576; 
		}

		// Audio
		virtual int get_audio_streams( ) const 
		{ 
			return 1; 
		}

	protected:
		// Fetch method
		void fetch( frame_type_ptr &result )
		{
			producer_->seek( get_position( ) );
			Mlt::Frame *mlt_native = producer_->get_frame( );
			if ( mlt_native )
			{
				has_video_ = mlt_native->get_int( "test_image" ) == 0;
				has_audio_ = mlt_native->get_int( "test_audio" ) == 0;
			}
			result = frame_type_ptr( new mlt_frame( mlt_native, get_position( ), get_process_flags( ) ) );
		}

		virtual bool initialize( )
		{
			producer_ = new Mlt::Producer( ( char * )( opl::to_string( media_ ).c_str( ) ) );
			if ( producer_ && producer_->is_valid( ) )
				fetch( );
			return producer_ && producer_->is_valid( );
		}

	private:
		const opl::wstring media_;
		Mlt::Producer *producer_;
		bool has_video_;
		bool has_audio_;
};

//
// Plugin object
//

class ML_PLUGIN_DECLSPEC mlt_plugin : public openmedialib_plugin
{
public:
	virtual input_type_ptr input( const opl::wstring& media )
	{
		input_type_ptr input = input_type_ptr( new mlt_input( media ) );
		if ( input->get_frames( ) > 0 )
			return input;
		else
			return input_type_ptr( );
	}

	virtual store_type_ptr store( const opl::wstring &, const frame_type_ptr & )
	{
		return store_type_ptr( );
	}
};

} } }

//
// Library initialisation mechanism
//

namespace
{
	boost::recursive_mutex mutex;

	void reflib( int init )
	{
		boost::recursive_mutex::scoped_lock lock( mutex );
		static long refs = 0;
		if( init > 0 && ++refs == 1 )
			Mlt::Factory::init( );
	}
}

//
// Access methods for openpluginlib
//

extern "C"
{
	ML_PLUGIN_DECLSPEC bool openplugin_init( void )
	{
		reflib( 1 );
		return true;
	}

	ML_PLUGIN_DECLSPEC bool openplugin_uninit( void )
	{
		reflib( -1 );
		return true;
	}
	
	ML_PLUGIN_DECLSPEC bool openplugin_create_plugin( const char*, opl::openplugin** plug )
	{
		*plug = new plugin::mlt_plugin;
		return true;
	}
	
	ML_PLUGIN_DECLSPEC void openplugin_destroy_plugin( opl::openplugin* plug )
	{ 
		delete static_cast< plugin::mlt_plugin * >( plug ); 
	}
}
