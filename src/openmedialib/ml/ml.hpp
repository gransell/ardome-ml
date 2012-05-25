
// ml - A media library representation.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef ML_INC_
#define ML_INC_

#include <openpluginlib/pl/string.hpp>

#include <openmedialib/ml/config.hpp>
#include <openmedialib/ml/types.hpp>
#include <openmedialib/ml/filter.hpp>
#include <openmedialib/ml/filter_simple.hpp>
#include <openmedialib/ml/frame.hpp>
#include <openmedialib/ml/input.hpp>
#include <openmedialib/ml/store.hpp>
#include <openmedialib/ml/utilities.hpp>
#include <openmedialib/ml/indexer.hpp>

namespace olib { namespace openmedialib { namespace ml {

ML_DECLSPEC bool init( );
ML_DECLSPEC bool uninit( );

} } }

#endif

