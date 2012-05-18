// ml - A media library representation.

// Copyright (C) 2011 Vizrt
// Released under the LGPL.

#ifndef ML_ANALYSE_DV
#define ML_ANALYSE_DV

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/analyse.hpp>

namespace olib { namespace openmedialib { namespace ml {

class ML_DECLSPEC analyse_dv : public analyse_type
{
	public:
		analyse_dv( );

		bool valid( const std::string &codec );

		bool analysis( boost::uint8_t *data, const size_t size );

		bool collect( olib::openpluginlib::pcos::property_container &properties );

	protected:
		int id( boost::uint8_t *data, const size_t size );

	private:
		int id_;
		bool is_wide_;
};

} } }

#endif
