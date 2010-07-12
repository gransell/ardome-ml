
#ifndef OPENMEDIALIB_FILTER_SIMPLE_INC_
#define OPENMEDIALIB_FILTER_SIMPLE_INC_

#include <openmedialib/ml/filter.hpp>

namespace olib { namespace openmedialib { namespace ml {

class ML_DECLSPEC filter_simple : public filter_type
{
	public:
		filter_simple( )
			: filter_type( )
			, total_frames_( 0 )
		{ }

		virtual ~filter_simple( )
		{ }

		int get_frames( ) const
		{ return total_frames_; }

	protected:
		void sync_frames( )
		{
			ml::input_type_ptr input = fetch_slot( 0 );
			total_frames_ = input ? input->get_frames( ) : 0;
		}

	private:
		int total_frames_;
};

} } }

#endif
