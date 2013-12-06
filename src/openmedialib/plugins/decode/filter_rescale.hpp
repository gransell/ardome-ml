// decode - A decode plugin to ml.
//
// Copyright (C) 2010 Vizrt
// Released under the LGPL.

#include <openmedialib/ml/filter_simple.hpp>

namespace pl = olib::openpluginlib;
namespace ml = olib::openmedialib::ml;
namespace il = olib::openimagelib::il;
namespace pcos = olib::openpluginlib::pcos;
namespace cl = olib::opencorelib;

namespace olib { namespace openmedialib { namespace ml { namespace decode {

class ML_PLUGIN_DECLSPEC filter_rescale : public filter_simple
{
	public:
		filter_rescale( );

		virtual ~filter_rescale( );

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const ;

		virtual const std::wstring get_uri( ) const ;

		virtual const size_t slot_count( ) const ;

	protected:

		void do_fetch( frame_type_ptr &frame );

	private:
		pl::pcos::property prop_enable_;
		pl::pcos::property prop_progressive_;
		pl::pcos::property prop_interp_;
		pl::pcos::property prop_width_;
		pl::pcos::property prop_height_;
		pl::pcos::property prop_sar_num_;
		pl::pcos::property prop_sar_den_;
		ml::frame_type_ptr last_frame_;
};

} } } }


