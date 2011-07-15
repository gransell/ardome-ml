
#include <cairo.h>
#include <boost/shared_ptr.hpp>
#include <opencorelib/cl/export_defines.hpp>

namespace aml { namespace external { namespace cairo {


class surface;
class font;
class context;

typedef boost::shared_ptr< surface > surface_ptr;
typedef boost::shared_ptr< context > context_ptr;


namespace slant {
typedef enum {
	normal  = CAIRO_FONT_SLANT_NORMAL,
	italic  = CAIRO_FONT_SLANT_ITALIC,
	oblique = CAIRO_FONT_SLANT_OBLIQUE
} type;
CORE_API type from_string( const std::string& s );
}

namespace weight {
typedef enum {
	normal  = CAIRO_FONT_WEIGHT_NORMAL,
	bold    = CAIRO_FONT_WEIGHT_BOLD
} type;
CORE_API type from_string( const std::string& s );
}

namespace origin {
typedef enum {
	top_left,
	top_right,
	bottom_left,
	bottom_right
} type;
CORE_API type from_string( const std::string& s );
}

namespace alignment {
typedef enum {
	right_up,
	right_middle,
	right_down,
	left_up,
	left_middle,
	left_down,
	center_up,
	center_middle,
	center_down
} type;
CORE_API type from_string( const std::string& s );
}


} } } // namespace cairo, namespace external, namespace aml

