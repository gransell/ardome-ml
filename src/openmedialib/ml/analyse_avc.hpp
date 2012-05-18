// ml - A media library representation.

// Copyright (C) 2011 Vizrt
// Released under the LGPL.

#ifndef ML_ANALYSE_AVC
#define ML_ANALYSE_AVC

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/analyse.hpp>

namespace olib { namespace openmedialib { namespace ml {

class ML_DECLSPEC analyse_avc : public analyse_type
{
	public:
		analyse_avc( ) { }

		bool valid( const std::string &codec ) { return true; }

		bool analysis( boost::uint8_t *data, const size_t size ) { return true; }

		bool collect( olib::openpluginlib::pcos::property_container &properties ) { return true; }
};

} } }

#endif
