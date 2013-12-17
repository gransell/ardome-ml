// decode - A decode plugin to ml.
//
// Copyright (C) 2010 Vizrt
// Released under the LGPL.

#include "filter_pool.hpp"
#include <openmedialib/ml/filter_encode.hpp>
#include <opencorelib/cl/profile_types.hpp>

namespace pl = olib::openpluginlib;
namespace ml = olib::openmedialib::ml;
namespace il = olib::openmedialib::ml;
namespace pcos = olib::openpluginlib::pcos;
namespace cl = olib::opencorelib;

namespace olib { namespace openmedialib { namespace ml { namespace decode {

class ML_PLUGIN_DECLSPEC filter_encode : public filter_encode_type, public filter_pool
{
	private:
		//Note: storing any shared_ptr:s to frames here (such as
		//a last-frame cache) will cause a cyclic reference chain
		//due to the shared_ptr that's in frame_lazy, so don't do that.
		boost::recursive_mutex mutex_;
		pl::pcos::property prop_filter_;
		pl::pcos::property prop_profile_;
		pl::pcos::property prop_instances_;
		pl::pcos::property prop_enable_;
		std::deque< ml::filter_type_ptr > decoder_;
		ml::filter_type_ptr gop_encoder_;
		bool is_long_gop_;
		bool initialized_;
		cl::profile_ptr profile_to_encoder_mappings_;
		bool stream_validation_;
 
	public:
		filter_encode( );

		virtual ~filter_encode( );

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const ;

		virtual const std::wstring get_uri( ) const ;

		virtual const size_t slot_count( ) const ;

	protected:

		ml::filter_type_ptr filter_create( );

 		ml::filter_type_ptr filter_obtain( );

		void filter_release( ml::filter_type_ptr filter );

		void push( ml::input_type_ptr input, ml::frame_type_ptr frame );

		bool create_pushers( );

		void do_fetch( frame_type_ptr &frame );

		void initialize_encoder_mapping( );
};

} } } }
