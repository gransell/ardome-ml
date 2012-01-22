#pragma once

namespace OWL {
	ref class Any;
}

namespace Olib { namespace Opencorelib {
	ref class RgbaColor;
} }

namespace OWL {
	[System::Runtime::CompilerServices::Extension]
	public ref class OWLAnyAMLExtensions abstract sealed
	{
		public:
			[System::Runtime::CompilerServices::Extension]
			static bool IsRgbaColor( OWL::Any ^any );

			[System::Runtime::CompilerServices::Extension]
			static Olib::Opencorelib::RgbaColor^ ValueAsRgbaColor( OWL::Any ^any );
	};
}

