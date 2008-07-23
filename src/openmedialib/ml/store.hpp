
// ml - A media library representation.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef OPENMEDIALIB_STORE_INC_
#define OPENMEDIALIB_STORE_INC_

#include <boost/shared_ptr.hpp>
#include <openmedialib/ml/frame.hpp>
#include <openpluginlib/pl/pcos/property_container.hpp>

namespace olib { namespace openmedialib { namespace ml {

// Keyboard feedback interface implementations
//
// See notes below.

class ML_DECLSPEC store_keyboard_handler
{
	public:
		virtual ~store_keyboard_handler( ) { }
		virtual void keyboard_handler( unsigned char ) = 0;
};

class ML_DECLSPEC store_keyboard_feedback
{
	public:
		virtual ~store_keyboard_feedback( ) { }
		virtual void register_keyboard_handler( store_keyboard_handler * ) = 0;
};

// Position feedback interface implementations - this one provides a frame for 
// the app to do something with immediately before presentation (so it could do 
// application specific image manipulation or position reporting etc).
//
// See notes below.

class ML_DECLSPEC store_position_handler
{
	public:
		virtual ~store_position_handler( ) { }
		virtual void position_handler( frame_type_ptr ) = 0;
};

class ML_DECLSPEC store_position_feedback
{
	public:
		virtual ~store_position_feedback( ) { }
		virtual void register_position_handler( store_position_handler * ) = 0;
};

// Stores are objects that receive frames and do something with them - examples being things 
// like previewers, transcoders, analysers and so on.
//
// The minimum that a store implementation must provide is a push method.
//
// An implementation could optionally provide interface implementations such as the 
// store_keyboard_feedback, and the application may choose to use those when available
// (see test/openmedialib/store for an example).
//
// ml::store_type_ptr store = ...;
// ml::store_keyboard_feedback *feedback = dynamic_cast< ml::store_keyboard_feedback * >( store.get( ) );
//
// For this to work with g++, it is essential that:
//
// 1) all apps link with -Wl,-E or -Wl,--export-dynamic
// 2) dlopens are loaded with RTLD_GLOBAL (this should be ensured by openpluginlib)

class ML_DECLSPEC store_type
{
	public:
		explicit store_type( )
			: properties_( )
		{ }

		virtual ~store_type( ) 
		{ }

		// Property object
		olib::openpluginlib::pcos::property_container properties( )
		{ return properties_; }

		// Convenience method
		olib::openpluginlib::pcos::property property( const char *name ) const
		{ return properties_.get_property_with_string( name ); }

		// Initialise method
		virtual bool init( )
		{ return true; }

		// Push a frame to the store
		virtual bool push( frame_type_ptr ) = 0;

		// Tell the store to flush all pending frames, while returning the
		// one that was pending. The default implementation applies when 
		// the store doesn't queue frames.
		virtual frame_type_ptr flush( )
		{ return frame_type_ptr( ); }

		// Playout all queued frames and return when done. The default
		// implementation applies when the store doesn't queue frames.
		virtual void complete( )
		{ }

		// Allows the store to dictate when it is running empty (ie: any
		// realtime store such as an audio player or device feed needs more
		// frames in order to provide smooth playout)
		virtual bool empty( )
		{ return false; }

	private:
		olib::openpluginlib::pcos::property_container properties_;
};

// The shared pointer type for a store
typedef boost::shared_ptr< store_type > store_type_ptr;

} } }

#endif
