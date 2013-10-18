#include <precompiled_headers.hpp>

#include "AMLConversionFunctions.hpp"

namespace OWL {

void AMLConversions::System_Windows_PointToolib_opencorelib_point(System::Windows::Point p, System::Int64 out_point_ptr)
{
	olib::opencorelib::point *point = reinterpret_cast<olib::opencorelib::point *>( out_point_ptr );
	*point = olib::opencorelib::point(p.X, p.Y);
}

System::Windows::Point AMLConversions::olib_opencorelib_pointToSystem_Windows_Point( System::Int64 point_ptr )
{
	olib::opencorelib::point *point = reinterpret_cast<olib::opencorelib::point *>( point_ptr );
	return System::Windows::Point(point->get_x(), point->get_y());
}

void AMLConversions::System_Int32Toolib_opencorelib_frames(System::Int32 i, System::Int64 out_frames_ptr )
{
	olib::opencorelib::frames *p = reinterpret_cast<olib::opencorelib::frames *>( out_frames_ptr );
	*p = olib::opencorelib::frames(i);
}

System::Int32 AMLConversions::olib_opencorelib_framesToSystem_Int32( System::Int64 frames_ptr )
{
	olib::opencorelib::frames *p = reinterpret_cast<olib::opencorelib::frames *>( frames_ptr );
	return System::Int32(p->get_frames());
}

Olib::Opencorelib::RationalTime AMLConversions::boost_rational___int64_ToOlib_Opencorelib_RationalTime(System::Int64 rational_ptr )
{
	boost::rational<boost::int64_t> *p = reinterpret_cast<boost::rational<boost::int64_t> *>( rational_ptr );
	return Olib::Opencorelib::RationalTime(p->numerator(), p->denominator());
}

void AMLConversions::Olib_Opencorelib_RationalTimeToboost_rational___int64_(Olib::Opencorelib::RationalTime r, System::Int64 out_rational_ptr )
{
	boost::rational<boost::int64_t> *p = reinterpret_cast<boost::rational<boost::int64_t> *>( out_rational_ptr );
	*p = boost::rational<boost::int64_t>(r.Num, r.Den);
}

}
