// decode - A decode plugin to ml.
//
// Copyright (C) 2010 Vizrt
// Released under the LGPL.

#ifndef plugin_decode_filter_map_reduce_h
#define plugin_decode_filter_map_reduce_h

#include <openmedialib/ml/filter_simple.hpp>
#include <opencorelib/cl/thread_pool.hpp>

namespace pl = olib::openpluginlib;
namespace ml = olib::openmedialib::ml;
namespace il = olib::openmedialib::ml;
namespace pcos = olib::openpluginlib::pcos;
namespace cl = olib::opencorelib;

namespace olib { namespace openmedialib { namespace ml { namespace decode {


class ML_PLUGIN_DECLSPEC filter_map_reduce : public filter_simple
{
	private:
		pl::pcos::property prop_threads_;
		long int frameno_;
		int threads_;
		std::map< int, frame_type_ptr > frame_map;
		cl::thread_pool *pool_;
		boost::recursive_mutex mutex_;
		boost::condition_variable_any cond_;
		ml::frame_type_ptr last_frame_;
		int expected_;
		int incr_;
		bool thread_safe_;

	public:
		filter_map_reduce( );

		virtual ~filter_map_reduce( );

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const;

		virtual const std::wstring get_uri( ) const;

		virtual const size_t slot_count( ) const;

	protected:

		void decode_job ( ml::frame_type_ptr frame );
 
		bool check_available( const int frameno );

		ml::frame_type_ptr wait_for_available( const int frameno );

		void make_unavailable( const int frameno );

		void make_available( ml::frame_type_ptr frame );

		void add_job( int frameno );

		void clear( );

		void do_fetch( frame_type_ptr &frame );
};

} } } }


#endif

