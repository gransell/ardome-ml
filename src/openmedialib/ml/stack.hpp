// Stack - a convenience wrapper for common use of the aml_stack input and aml filter

// Copyright (C) 2011 Vizrt
// Released under the LGPL.

#ifndef ML_STACK_H_
#define ML_STACK_H_

#include "types.hpp"
#include "utilities.hpp"

namespace olib { namespace openmedialib { namespace ml {

class stack
{
	public:
		stack( )
		: stack_( create_input( L"aml_stack:" ) )
		{
		}

		void push( input_type_ptr &input )
		{
			stack_->connect( input );
			push( L"recover" );
		}

		void push( filter_type_ptr &input )
		{
			stack_->connect( input );
			push( L"recover" );
		}

		void push( olib::openpluginlib::wstring command )
		{
			stack_->property( "parse" ) = command;
		}

		void copy( input_type_ptr &input, int limit = 1 )
		{
			filter_type_ptr aml = create_filter( L"aml" );
			aml->property( "limit" ) = limit;
			aml->property( "filename" ) = olib::openpluginlib::wstring( L"@" );
			aml->connect( input );
			olib::openpluginlib::wstring output = olib::openpluginlib::to_wstring( aml->property( "stdout" ).value< std::string >( ) );
			push( output );
		}

		input_type_ptr pop( )
		{
			push( L"." );
			return stack_->fetch_slot( 0 );
		}

	private:
		input_type_ptr stack_;
};

} } }

#endif
