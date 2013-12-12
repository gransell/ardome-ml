
// ml - A media library representation.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef OPENMEDIALIB_FRAME_INC_
#define OPENMEDIALIB_FRAME_INC_

#include <openmedialib/ml/types.hpp>
#include <openmedialib/ml/audio.hpp>
#include <openmedialib/ml/stream.hpp>
//#include <openmedialib/ml/image/image.hpp>

#include <boost/shared_ptr.hpp>
#include <openpluginlib/pl/pcos/property_container.hpp>

#include <deque>
#include <vector>
#include <utility>

namespace olib { namespace openmedialib { namespace ml {

	class ML_DECLSPEC frame_type : boost::noncopyable
{
	public:
		/// Constructor
		explicit frame_type( );

		/// Destructor
		virtual ~frame_type( );

		/// Provide a shallow copy of the frame (and all attached frames)
		virtual frame_type_ptr shallow( ) const;

		/// Provide a deepy copy of the frame (and a shallow copy of all attached frames)
		virtual frame_type_ptr deep( );

		/// Obtains the queue of frames (when there are no attached frames, the queue
		/// only contains a single frame, being the input).
		static std::deque< frame_type_ptr > unfold( frame_type_ptr );

		/// Convert a queue of frames to a single frame with the others attached (first 
		/// frame in the queue is modified and returned).
		static frame_type_ptr fold( std::deque< frame_type_ptr > & );

		/// Return the properties associated to this frame.
		olib::openpluginlib::pcos::property_container &properties( );

		/// Convenience method - returns the property by its name.
		olib::openpluginlib::pcos::property property( const char *name ) const;

		/// Convenience method - returns the property by its key.
		olib::openpluginlib::pcos::property property_with_key( const olib::openpluginlib::pcos::key &name ) const
		{ return properties_.get_property_with_key( name ); }

		/// Indicates if the frame has an image
		virtual bool has_image( );

		/// Indicates if the frame has audio
		virtual bool has_audio( );

		/// Set the image associated to the frame.
		/// decoded should only be set to true if the image
		/// object is a decoded representation of the stream in the frame
		virtual void set_image( olib::openmedialib::ml::image_type_ptr image, bool decoded = false );

		/// Set the packet associated to the frame.
		virtual void set_stream( olib::openmedialib::ml::stream_type_ptr );

		/// Get the packet associated to the frame.
		virtual olib::openmedialib::ml::stream_type_ptr get_stream( );

		/// Get the image associated to the frame.
		virtual olib::openmedialib::ml::image_type_ptr get_image( );

		/// Set the alpha mask associated to the frame.
		virtual void set_alpha( olib::openmedialib::ml::image_type_ptr image );

		/// Get the alpha mask associated to the frame.
		virtual olib::openmedialib::ml::image_type_ptr get_alpha( );

		/// Set the audio associated to the frame.
		/// decoded should only be set to true if the audio
		/// object is a decoded representation of the stream in the frame
		virtual void set_audio( audio_type_ptr audio, bool decoded = false );

		/// Get the audio associated to the frame.
		virtual audio_type_ptr get_audio( );

		/// Set the presentation time stamp.
		virtual void set_pts( double pts );

		/// Get the presentation time stamp.
		virtual double get_pts( ) const;

		/// Set the position of the frame.
		virtual void set_position( int position );

		/// Get the position of the frame.
		virtual int get_position( ) const;

		/// Set the duration of the frame.
		virtual void set_duration( double duration );

		/// Get the duration of the frame.
		virtual double get_duration( ) const;

		/// Set the sample aspect ratio of the frame
		virtual void set_sar( int num, int den );

		/// Get the sample aspect ratio of the frame
		virtual void get_sar( int &num, int &den ) const;

		/// Set the frames per specond ratio of the frame
		virtual void set_fps( int num, int den );

		/// Get the frames per specond ratio of the frame
		virtual void get_fps( int &num, int &den ) const;

		/// Convenience - return fps numerator
		virtual int get_fps_num( );

		/// Convenience - return fps denominator
		virtual int get_fps_den( );

		/// Convenience - return fps denominator
		virtual int get_sar_num( );

		/// Convenience - return fps denominator
		virtual int get_sar_den( );

		/// Convenience - return the full aspect ratio of the frame
		virtual double aspect_ratio( );

		/// Convenience - return the fps of the frame
		virtual double fps( ) const;

		/// Convenience - return the full aspect ratio of the frame
		virtual double sar( ) const;

		/// Attach a frame to this frame (for example, in deferred rendering mode, 
		/// we would attach all foreground frames to the background).
		void push( frame_type_ptr frame );

		/// Pop the next frame associated to this one.
		frame_type_ptr pop( );

		/// Register an exception with the frame.
		void push_exception( exception_ptr exception, input_type_ptr input );

		/// Return the list of exceptions.
		exception_list exceptions( ) const;

		/// Convenience - check if the frame has any associated exceptions.
		const bool in_error( ) const;

		/// Work around for weak_ptr failure in python
		void clear_exceptions( );

		/// Indicates the container format if known ("" if unknown or n/a)
		virtual std::string container( ) const;

		/// Indicates the input's video codec if known ("" if unknown or n/a)
		virtual std::string video_codec( ) const;

		/// Obtain width from packet or image
		virtual int width( ) const;

		/// Obtain height from packet or image
		virtual int height( ) const;

		/// Obtain colourspace from packet or image
		t_string pf( ) const;

		/// Obtain field order from packet or image
		virtual olib::openmedialib::ml::image::field_order_flags field_order( ) const;

		/// Indicates the input's audio codec if known ("" if unknown or n/a)
		virtual std::string audio_codec( ) const;

		/// Obtain frequency from packet or audio
		int frequency( ) const;

		/// Obtain channels from packet or audio
		int channels( ) const;

		/// Obtain samples from packet or audio
		int samples( ) const;

		/// Indicates if the frame is deferred
		bool is_deferred( ) const { return queue_.size( ) > 0; }

		/// Return the deferred frames associated to this frame
		std::deque< frame_type_ptr > queue( ) const { return queue_; }

		/// Return the list of audio stream components associated to this frame
		void set_audio_block( const audio::block_type_ptr &block ) { audio_block_ = block; }

		/// Return the list of audio stream components associated to this frame
		audio::block_type_ptr &audio_block( ) { return audio_block_; }

		/// Return the const list of audio stream components associated to this frame
		const audio::block_type_ptr &audio_block( ) const { return audio_block_; }

		/// Get the image associated to the frame. Just return the member variable without triggering the decoding from stream.
		const olib::openmedialib::ml::image_type_ptr get_evaluated_image( ) const { return image_; }

		/// Returns the enumerated form of the picture format
		image::MLPixelFormat ml_pixel_format( ) const;

		/// Indicates if the image associated to the frame is yuv planar image
		bool is_yuv_planar( ) const;

		/// Indicates if the frame has an alpha component
		bool has_alpha( ) const;

	protected:
	
		/// Copy constructor from a frame_type_ptr
		frame_type( const frame_type *other );
	
		olib::openpluginlib::pcos::property_container properties_;
		olib::openmedialib::ml::stream_type_ptr stream_;
		olib::openmedialib::ml::image_type_ptr image_;
		olib::openmedialib::ml::image_type_ptr alpha_;
		audio_type_ptr audio_;
		double pts_;
		int position_;
		double duration_;
		int sar_num_;
		int sar_den_;
		int fps_num_;
		int fps_den_;
		std::deque< frame_type_ptr > queue_;
		exception_list exceptions_;
		audio::block_type_ptr audio_block_;
};

} } }

#endif
