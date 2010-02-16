#pragma once

#include "opencorelib/cl/core.hpp"
#include "opencorelib/cl/point.hpp"
#include "opencorelib/cl/frames.hpp"

namespace OWL { 
	
	public ref class AMLConversions
	{
	public:
		static void System_Windows_PointToolib_opencorelib_point( System::Windows::Point p, System::Int64 out_point_ptr);
		static System::Windows::Point olib_opencorelib_pointToSystem_Windows_Point( System::Int64 point_ptr);

		static void System_Int32Toolib_opencorelib_frames(System::Int32 i, System::Int64 out_frames_ptr );
		static System::Int32 olib_opencorelib_framesToSystem_Int32( System::Int64 frames_ptr );

		static Olib::Opencorelib::RationalTime boost_rational___int64_ToOlib_Opencorelib_RationalTime(System::Int64 rational_ptr );
		static void Olib_Opencorelib_RationalTimeToboost_rational___int64_(Olib::Opencorelib::RationalTime r, System::Int64 out_rational_ptr );
	};
}