// ml - A media library representation.

// Copyright (C) 2009 Ardendo
// Released under the LGPL.

#ifndef OPENMEDIALIB_INDEXER_H_
#define OPENMEDIALIB_INDEXER_H_

#include <openmedialib/ml/types.hpp>
#include <openmedialib/ml/awi.hpp>
#include <openpluginlib/pl/string.hpp>

namespace olib { namespace openmedialib { namespace ml {

class ML_DECLSPEC indexer_item
{
	public:
		/// Provides the awi index object associated to this item
		virtual awi_index_ptr index( ) = 0;

		/// Reports the current size of the media associated to this item
		virtual const boost::int64_t size( ) const = 0;

		/// Indicates if growth has completed or not
		virtual const bool finished( ) const = 0;
};

// Initialise the indexer
extern void ML_DECLSPEC indexer_init( );

/// Factory method for index_item_ptr objects
extern indexer_item_ptr ML_DECLSPEC indexer_request( const openpluginlib::wstring &url );

/// Shuts the indexer subsystem down
extern void ML_DECLSPEC indexer_shutdown( );

} } }

#endif
