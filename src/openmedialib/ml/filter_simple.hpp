
#ifndef OPENMEDIALIB_FILTER_SIMPLE_INC_
#define OPENMEDIALIB_FILTER_SIMPLE_INC_

#include <openmedialib/ml/filter.hpp>
#include <boost/thread.hpp>

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
		{ 
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			return total_frames_; 
		}

	protected:
		void sync_frames( )
		{
			if ( sync_mutex_.try_lock( ) )
			{
				boost::recursive_mutex::scoped_lock lock( mutex_ );
				ml::input_type_ptr input = fetch_slot( 0 );
				total_frames_ = input ? input->get_frames( ) : 0;
				sync_mutex_.unlock( );
			}
		}

	private:
		mutable boost::recursive_mutex mutex_;
		boost::recursive_mutex sync_mutex_;
		int total_frames_;
};

} } }

#endif
