#ifndef plugin_decode_filter_decode_h
#define plugin_decode_filter_decode_h

#include "filter_pool.hpp"
#include <openmedialib/ml/filter.hpp>
#include <opencorelib/cl/profile_types.hpp>
#include <boost/enable_shared_from_this.hpp>

namespace pl = olib::openpluginlib;
namespace ml = olib::openmedialib::ml;
namespace il = olib::openmedialib::ml;
namespace pcos = olib::openpluginlib::pcos;
namespace cl = olib::opencorelib;

namespace olib { namespace openmedialib { namespace ml { namespace decode {

class ML_PLUGIN_DECLSPEC filter_decode : public filter_type, public filter_pool
{
	private:
		//Note: storing any shared_ptr:s to frames here (such as
		//a last-frame cache) will cause a cyclic reference chain
		//due to the shared_ptr that's in frame_lazy, so don't do that.
		boost::recursive_mutex mutex_;
		pl::pcos::property prop_inner_threads_;
		pl::pcos::property prop_filter_;
		pl::pcos::property prop_audio_filter_;
		pl::pcos::property prop_scope_;
		pl::pcos::property prop_source_uri_;
		std::deque< ml::filter_type_ptr > decoder_;
		ml::filter_type_ptr gop_decoder_;
		ml::filter_type_ptr audio_decoder_;
		cl::profile_ptr codec_to_decoder_;
		bool initialized_;
		std::string video_codec_;
		int total_frames_;
		boost::int64_t precharged_frames_;
		mutable int estimated_gop_size_;

	public:
		filter_decode( );

		virtual ~filter_decode( );

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const ;

		virtual const std::wstring get_uri( ) const ;

		virtual const size_t slot_count( ) const ;

		void sync( ) ;

	protected:

		ml::filter_type_ptr filter_create( const std::wstring& codec_filter ) ;

 		ml::filter_type_ptr filter_obtain( ) ;
	
		void filter_release( ml::filter_type_ptr filter ) ;

		bool determine_decode_use( ml::frame_type_ptr &frame ) ;

		void do_fetch( frame_type_ptr &frame ) ;

		virtual int get_frames( ) const ;

		void initialize_video( ml::frame_type_ptr &first_frame ) ;

		void initialize_audio( ml::frame_type_ptr &first_frame ) ;

	private:
	
		frame_type_ptr perform_audio_decode( const frame_type_ptr& frame ) ;

		static int calculate_estimated_gop_size( const frame_type_ptr &frame ) ;
	
		int estimated_gop_size( ) const ;

		void resolve_inner_threads_prop( ) ;
};

} } } }

#endif

