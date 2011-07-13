
#include <cairo.h>
#include <boost/shared_ptr.hpp>

namespace aml { namespace openmedialib { namespace cairo {


class surface;
class font;
class context;

typedef boost::shared_ptr< surface > surface_ptr;
typedef boost::shared_ptr< context > context_ptr;


namespace slant { typedef enum {
	normal  = CAIRO_FONT_SLANT_NORMAL,
	italic  = CAIRO_FONT_SLANT_ITALIC,
	oblique = CAIRO_FONT_SLANT_OBLIQUE
} type; }

namespace weight { typedef enum {
	normal  = CAIRO_FONT_WEIGHT_NORMAL,
	bold    = CAIRO_FONT_WEIGHT_BOLD
} type; }

namespace origin { typedef enum {
	top_left,
	top_center,
	top_right,
	middle_left,
	middle_center,
	middle_right,
	bottom_left,
	bottom_center,
	bottom_right
} type; }

namespace alignment { typedef enum {
	right_down,
	right_up,
	left_down,
	left_up
} type; }


} } } // namespace cairo, namespace openmedialib, namespace aml

