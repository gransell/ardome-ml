// ml - A media library representation.

// Copyright (C) 2009 Ardendo
// Released under the LGPL.

#ifndef OPENMEDIALIB_INDEXER_H_
#define OPENMEDIALIB_INDEXER_H_

#include <openmedialib/ml/types.hpp>
#include <openmedialib/ml/awi.hpp>

namespace olib { namespace openmedialib { namespace ml {

namespace index_type
{
	enum type
	{
		awi,
		media
	};
}

class ML_DECLSPEC indexer_item
{
	public:
		// Virtual dtor
		virtual ~indexer_item( ) { }

		/// Provides the awi index object associated to this item
		virtual awi_index_ptr index( ) = 0;

		/// Reports the current size of the media associated to this item
		virtual const boost::int64_t size( ) const = 0;

		/// Indicates if growth has completed or not
		virtual const bool finished( ) const = 0;

		// V4 indexes can index both audio and video
		virtual const boost::uint16_t type( ) const { return 0; }
};

// Initialise the indexer
extern void ML_DECLSPEC indexer_init( );

/// Factory method for index_item_ptr objects
extern indexer_item_ptr ML_DECLSPEC indexer_request( const std::wstring &url, 
													index_type::type file_type,
													boost::uint16_t v4_index_entry_type = 0 );

/// Cancels an indexer request previously returned by indexer_request
extern void ML_DECLSPEC indexer_cancel_request( const indexer_item_ptr &item );

/// Shuts the indexer subsystem down
extern void ML_DECLSPEC indexer_shutdown( );

} } }

#endif
