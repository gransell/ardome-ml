// ml - A media library representation.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <openpluginlib/py/python.hpp>
#include <openmedialib/py/py.hpp>

namespace ml = olib::openmedialib::ml;

BOOST_PYTHON_MODULE( openmedialib )
{
	ml::detail::py_audio( );
	ml::detail::py_audio_reseat( );
	ml::detail::py_stream_type( );
	ml::detail::py_frame( );
	ml::detail::py_input( );
	ml::detail::py_store( );
	ml::detail::py_filter( );
	ml::detail::py_plugin( );
	ml::detail::py_stream( );
	ml::detail::py_stack( );
}
