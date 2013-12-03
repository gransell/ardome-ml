// decode - A decode plugin to ml.
//
// Copyright (C) 2010 Vizrt
// Released under the LGPL.

#ifndef plugin_decode_filter_lazy_h
#define plugin_decode_filter_lazy_h

#include "filter_pool.hpp"
#include <openmedialib/ml/filter.hpp>

namespace olib { namespace openmedialib { namespace ml { namespace decode {

class ML_PLUGIN_DECLSPEC filter_lazy : public filter_type, public filter_pool
{
	public:
		filter_lazy( const std::wstring &spec );

		virtual ~filter_lazy( );

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const;

		virtual const std::wstring get_uri( ) const ;

		virtual const size_t slot_count( ) const ;

		virtual bool is_thread_safe( );

		void on_slot_change( input_type_ptr input, int slot );

		int get_frames( ) const;

 		ml::filter_type_ptr filter_obtain( );

		ml::filter_type_ptr filter_create( );

		void filter_release( ml::filter_type_ptr filter );

		void do_fetch( frame_type_ptr &frame );

		void sync_frames( );

	private:
		boost::recursive_mutex mutex_;
		std::wstring spec_;
		ml::filter_type_ptr filter_;
		std::deque< ml::filter_type_ptr > filters_;
		ml::frame_type_ptr last_frame_;
};


} } } }

#endif

