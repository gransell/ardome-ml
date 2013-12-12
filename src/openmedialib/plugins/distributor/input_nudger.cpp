
//
// #input:nudger:
//
// This provides an upgrade to the original input:pusher: - it provides a 
// threadsafe implementation which allows pushes and fetches to be carried out
// on two different threads.
//
#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/keys.hpp>
#include <opencorelib/cl/enforce_defines.hpp>
#include <opencorelib/cl/log_defines.hpp>

namespace pl = olib::openpluginlib;
namespace ml = olib::openmedialib::ml;
namespace cl = olib::opencorelib;

namespace olib { namespace openmedialib { namespace ml { namespace distributor {

class ML_PLUGIN_DECLSPEC input_nudger : public input_type
{
	public:
		input_nudger( ) 
		: prop_length_( pl::pcos::key::from_string( "length" ) )
		, prop_queue_( pl::pcos::key::from_string( "queue" ) )
		, prop_timeout_( pl::pcos::key::from_string( "timeout" ) )
		{ 
			properties( ).append( prop_length_ = INT_MAX );
			properties( ).append( prop_queue_ = 50 );
			properties( ).append( prop_timeout_ = 5000 );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		// Basic information
		virtual const std::wstring get_uri( ) const { return L"nudger:"; }
		virtual const std::wstring get_mime_type( ) const { return L""; }
		virtual bool is_seekable( ) const { return true; }

		// Audio/Visual
		virtual int get_frames( ) const { return prop_length_.value< int >( ); }

		// Push method
		virtual bool push( frame_type_ptr frame )
		{
			lru_.resize( prop_queue_.value< int >( ) );
			if ( frame )
				lru_.append( frame->get_position( ), frame );
			else
				lru_.clear( );
			return true;
		}

	protected:
		// Fetch method
		void do_fetch( frame_type_ptr &result )
		{
			result = lru_.wait( get_position( ), boost::posix_time::milliseconds( prop_timeout_.value< int >( ) ));
			if ( result )
				result = result->shallow( );
		}

	private:
		pl::pcos::property prop_length_;
		pl::pcos::property prop_queue_;
		pl::pcos::property prop_timeout_;
		ml::lru_frame_type lru_;
};

input_type_ptr create_nudger()
{
	return input_type_ptr( new input_nudger() );
}

} } } }
