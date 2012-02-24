#include <precompiled_headers.hpp>

#include "./OWLAnyAMLExtensions.hpp"
#include "Olib_Opencorelib_RgbaColor.hpp"

namespace OWL {

	namespace {
		template<typename T>
		T checked_any_cast( const boost::any *a )
		{
			try {
				return boost::any_cast<T>(*a);
			}
			catch ( const boost::bad_any_cast &e )
			{
				std::string msg( "Invalid type cast of Any: " );
				throw gcnew System::InvalidCastException( gcnew System::String((msg + e.what()).c_str()));
			}
		}
	}

	bool OWLAnyAMLExtensions::IsRgbaColor( OWL::Any ^any )
	{
		boost::any *a = reinterpret_cast<boost::any*>( any->__owl_get_internal_any_ptr() );
		return a->type() == typeid(olib::opencorelib::rgba_color);
	}

	Olib::Opencorelib::RgbaColor^ OWLAnyAMLExtensions::ValueAsRgbaColor( OWL::Any ^any )
	{
		boost::any *a = reinterpret_cast<boost::any*>( any->__owl_get_internal_any_ptr() );
		return Detail::the_rgba_color_wrapper::WrapAsStackObject( reinterpret_cast<boost::int64_t>( &checked_any_cast<const olib::opencorelib::rgba_color &>(a) ) );
	}
}

