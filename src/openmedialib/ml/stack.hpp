// Stack - a convenience wrapper for common use of the aml_stack input and aml filter

// Copyright (C) 2011 Vizrt
// Released under the LGPL.

#ifndef ML_STACK_H_
#define ML_STACK_H_

#include "types.hpp"
#include "utilities.hpp"
#include <opencorelib/cl/str_util.hpp>
#include <opencorelib/cl/enforce_defines.hpp>

namespace olib { namespace openmedialib { namespace ml {

class stack
{
	public:
		stack( )
		: stack_( create_input( L"aml_stack:" ) )
		{
		}

		stack &push( const std::wstring command )
		{
			stack_->property( "parse" ) = command;
			ARENFORCE_MSG( result( ) == "OK", "Stack push failed: %s" )( result( ) );
			return *this;
		}

		input_type_ptr release( )
		{
			stack_->property( "parse" ) = std::wstring( L"." );
			input_type_ptr result = fetch_slot( );
			connect( input_type_ptr( ) );
			return result;
		}

		input_type_ptr pop( )
		{
			return push( L"." ).fetch_slot( );
		}

		stack &push( const char **argv, int argc, int start = 0 )
		{
			for( int index = start; index < argc; index ++ )
				push( argv[ index ] );
			return *this;
		}

		stack &push( const std::string command )
		{
			return push( olib::opencorelib::str_util::to_wstring( command ) );
		}

		stack &push( const char *command )
		{
			return push( olib::opencorelib::str_util::to_wstring( command ) );
		}

		stack &push( input_type_ptr input )
		{
			return connect( input ).push( L"recover" );
		}

		stack &copy( input_type_ptr &input, int limit = 1 )
		{
			filter_type_ptr aml = create_filter( L"aml" );
			aml->property( "limit" ) = limit;
			aml->property( "filename" ) = std::wstring( L"@" );
			aml->connect( input );
			return push( olib::opencorelib::str_util::to_wstring( aml->property( "stdout" ).value< std::string >( ) ) );
		}

	private:
		std::string result( )
		{
			return olib::opencorelib::str_util::to_string( stack_->property( "result" ).value< std::wstring >( ) );
		}

		input_type_ptr fetch_slot( )
		{
			return stack_->fetch_slot( );
		}

		stack &connect( input_type_ptr input )
		{
			stack_->connect( input );
			return *this;
		}

		input_type_ptr stack_;
};

} } }

#endif
