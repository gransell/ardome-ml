
#include "cairo_types.hpp"

#include <opencorelib/cl/enforce_defines.hpp>

namespace aml { namespace external { namespace cairo {

#define TRY_NAME( name ) \
	if ( s == #name ) \
		return name
#define TRY_NAME_DEFAULT( name ) \
	if ( s.empty( ) || s == #name ) \
		return name

slant::type slant::from_string( const std::string& s ) {
	TRY_NAME_DEFAULT( normal );
	TRY_NAME( italic );
	TRY_NAME( oblique );
	ARENFORCE_MSG( false, "No such slant: \"%1%\"" )( s );
	return normal; // To hide compiler warnings
}

weight::type weight::from_string( const std::string& s )
{
	TRY_NAME_DEFAULT( normal );
	TRY_NAME( bold );
	ARENFORCE_MSG( false, "No such weight: \"%1%\"" )( s );
	return normal; // To hide compiler warnings
}

origin::type origin::from_string( const std::string& s )
{
	TRY_NAME_DEFAULT( top_left );
	TRY_NAME( top_center );
	TRY_NAME( top_right );
	TRY_NAME( middle_left );
	TRY_NAME( middle_center );
	TRY_NAME( middle_right );
	TRY_NAME( bottom_left );
	TRY_NAME( bottom_center );
	TRY_NAME( bottom_right );
	ARENFORCE_MSG( false, "No such origin: \"%1%\"" )( s );
	return top_left; // To hide compiler warnings
}

alignment::type alignment::from_string( const std::string& s )
{
	TRY_NAME_DEFAULT( right_down );
	TRY_NAME( right_up );
	TRY_NAME( left_down );
	TRY_NAME( left_up );
	ARENFORCE_MSG( false, "No such alignment: \"%1%\"" )( s );
	return right_down; // To hide compiler warnings
}


} } } // namespace cairo, namespace external, namespace aml


