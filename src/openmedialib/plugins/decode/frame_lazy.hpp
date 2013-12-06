// Copyright (C) 2010 Vizrt
// Released under the LGPL.

#ifndef plugin_decode_frame_lazy_h
#define plugin_decode_frame_lazy_h

#include "filter_pool.hpp"
#include <openmedialib/ml/frame.hpp>

namespace pl = olib::openpluginlib;
namespace ml = olib::openmedialib::ml;
namespace il = olib::openimagelib::il;
namespace pcos = olib::openpluginlib::pcos;
namespace cl = olib::opencorelib;

namespace olib { namespace openmedialib { namespace ml { namespace decode {

//If you add members here, make sure to update component_to_string() in
//frame_lazy.cpp as well.
typedef enum
{
	eval_image = 1,
	eval_stream = 2
}
component;

class ML_PLUGIN_DECLSPEC frame_lazy : public ml::frame_type 
{
	public:
		/// Constructor
		frame_lazy( const frame_type_ptr &other, int frames, const filter_pool_ptr &filter_pool, bool validated = true );

		/// Destructor
		virtual ~frame_lazy( );

		void evaluate( component comp );

		/// Provide a shallow copy of the frame (and all attached frames)
		virtual frame_type_ptr shallow( ) const;

		/// Provide a deep copy of the frame (and all attached frames)
		virtual frame_type_ptr deep( );

		/// Indicates if the frame has an image
		virtual bool has_image( );

		/// Indicates if the frame has audio
		virtual bool has_audio( );

		/// Set the image associated to the frame.
		virtual void set_image( olib::openimagelib::il::image_type_ptr image, bool decoded );

		/// Get the image associated to the frame.
		virtual olib::openimagelib::il::image_type_ptr get_image( );

		/// Set the audio associated to the frame.
		virtual void set_audio( audio_type_ptr audio, bool decoded );

		/// Get the audio associated to the frame.
		virtual audio_type_ptr get_audio( );

		virtual void set_stream( stream_type_ptr stream );

		virtual stream_type_ptr get_stream( );

		virtual void set_position( int position );

	protected:
		
		frame_lazy( const frame_lazy *org, const frame_type_ptr &other );

	private:
		ml::frame_type_ptr parent_;
		int frames_;
		bool validated_;
		int evaluated_;
		//Note: this is a shared pointer in order to keep the decoder/encoder
		//filter alive for the lifetime of this frame.
		//Otherwise, calling get_image() or get_stream() on the frame would
		//fail if the decode/encode filter that created it has gone out of
		//scope.
		//This also means that the decode/encode filters may not keep
		//shared pointers to frame_lazy instances as members, as that would
		//create a cyclic reference chain.
		filter_pool_ptr filter_pool_;
};

} } } }

#endif

