
// ml - A media library representation.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef OPENMEDIALIB_FRAME_INC_
#define OPENMEDIALIB_FRAME_INC_

#include <openimagelib/il/il.hpp>
#include <openmedialib/ml/audio.hpp>
#include <boost/shared_ptr.hpp>
#include <openpluginlib/pl/pcos/property_container.hpp>

#include <deque>

namespace olib { namespace openmedialib { namespace ml {

namespace pcos = olib::openpluginlib::pcos;

typedef ML_DECLSPEC olib::openimagelib::il::image_type image_type;
typedef ML_DECLSPEC olib::openimagelib::il::image_type_ptr image_type_ptr;

class ML_DECLSPEC frame_type;
typedef ML_DECLSPEC boost::shared_ptr< frame_type > frame_type_ptr;

class ML_DECLSPEC frame_type 
{
	public:
		explicit frame_type( );
		virtual ~frame_type( );
		static frame_type_ptr shallow_copy( const frame_type_ptr &other );
		static frame_type_ptr deep_copy( const frame_type_ptr &other );
		static std::deque< frame_type_ptr > unfold( frame_type_ptr );
		static frame_type_ptr fold( std::deque< frame_type_ptr > & );
		pcos::property_container properties( );
		pcos::property property( const char *name ) const;
		virtual void set_image( image_type_ptr image );
		virtual image_type_ptr get_image( );
		virtual void set_alpha( image_type_ptr image );
		virtual image_type_ptr get_alpha( );
		virtual void set_audio( audio_type_ptr audio );
		virtual audio_type_ptr get_audio( );
		virtual void set_pts( double pts );
		virtual double get_pts( ) const;
		virtual void set_position( int position );
		virtual int get_position( ) const;
		virtual void set_duration( double duration );
		virtual double get_duration( ) const;
		virtual void set_sar( int num, int den );
		virtual void get_sar( int &num, int &den ) const;
		virtual void set_fps( int num, int den );
		virtual void get_fps( int &num, int &den ) const;
		virtual int get_fps_num( );
		virtual int get_fps_den( );
		virtual int get_sar_num( );
		virtual int get_sar_den( );
		virtual double aspect_ratio( );
		virtual double fps( ) const;
		virtual double sar( ) const;
		void push( frame_type_ptr frame );
		frame_type_ptr pop( );

	private:
		pcos::property_container properties_;
		image_type_ptr image_;
		image_type_ptr alpha_;
		audio_type_ptr audio_;
		double pts_;
		int position_;
		double duration_;
		int sar_num_;
		int sar_den_;
		int fps_num_;
		int fps_den_;
		std::deque< frame_type_ptr > queue_;
};

} } }

#endif
