// Store filter
//
// Copyright (C) 2009 Ardendo

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"

#include <iostream>
#include <sstream>
#include <fstream>
#include <map>

#include <boost/cstdint.hpp>

namespace aml { namespace openmedialib {

#define const_slots const_cast< filter_slots * >

class ML_PLUGIN_DECLSPEC filter_slots : public ml::filter_type
{
	public:
		typedef fn_observer< filter_slots > observer;

		filter_slots( )
			: ml::filter_type( )
			, prop_in_( pcos::key::from_string( "in" ) )
			, prop_out_( pcos::key::from_string( "out" ) )
			, prop_original_input_( pcos::key::from_string( "original_input" ) )
			, prop_serialise_( pcos::key::from_string( "serialise" ) )
			, prop_serialise_terminator_( pcos::key::from_string( "serialise_terminator" ) )
			, obs_serialise_( new observer( const_slots( this ), &filter_slots::serialise ) )
		{
			properties( ).append( prop_in_ = 0 );
			properties( ).append( prop_out_ = -1 );
			properties( ).append( prop_original_input_ = ml::input_type_ptr( ) );
			properties( ).append( prop_serialise_ = boost::int64_t( 0 ) );
			properties( ).append( prop_serialise_terminator_ = 1 );
			prop_serialise_.attach( obs_serialise_ );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		virtual const size_t slot_count( ) const { return size_t( slots( ) ); }

		virtual const pl::wstring get_uri( ) const { return L"slots"; }

		virtual int get_frames( ) const { return frame_count( ); }

		/// Connect an input to a slot
		bool connect( ml::input_type_ptr input, size_t slot = 0 )
		{
			bool result = true;

			if ( inner_ )
			{
				int in, out, step;
				get_in_out( in, out, step );
				result = inner_->connect( input, size_t( in + int( slot ) * step ) );
			}
			else
			{
				inner_ = input;
				prop_original_input_ = inner_;
			}

			return result;
		}

		/// Retrieve the input associated to a slot
		ml::input_type_ptr fetch_slot( size_t slot = 0 ) const
		{
			if ( inner_ )
			{
				int in, out, step;
				get_in_out( in, out, step );
				return inner_->fetch_slot( size_t( in + int( slot ) * step ) );
			}
			return ml::input_type_ptr( );
		}

		void serialise( )
		{
			if ( prop_serialise_.value< boost::int64_t >( ) == boost::int64_t( 0 ) ) return;

			std::ostream *stream = reinterpret_cast< std::ostream * >( prop_serialise_.value< boost::int64_t >( ) );
			if ( inner_ )
			{
				ml::filter_type_ptr aml = ml::create_filter( L"aml" );
				aml->property( "filename" ) = pl::wstring( L"@" );
				aml->connect( inner_, 0 );
				*stream << aml->property( "stdout" ).value< std::string >( );
			}
			*stream << "filter:slots in=" << prop_in_.value< int >( ) << " out=" << prop_out_.value< int >( ) << std::endl;
		}

	protected:
		void do_fetch( ml::frame_type_ptr &result )
		{
			size_t slot = slot_for_position( get_position( ) );
			ml::input_type_ptr input = fetch_slot( slot );

			if ( input )
			{
				input->seek( get_position( ) - slot_offset( slot ) );
				result = input->fetch( );
				if ( result )
					result->set_position( get_position( ) );
			}
		}

	private:
		int slots( ) const
		{
			if ( inner_ )
			{
				int in, out, step;
				get_in_out( in, out, step );
				return size_t( ( out - in ) * step );
			}
			else
			{
				return 1;
			}
		}

		int frame_count( ) const
		{
			if ( inner_ )
			{
				return inner_->get_frames( );
			}
			else
			{
				return 0;
			}
		}

		void get_in_out( int &in, int &out, int &step ) const
		{
			in = prop_in_.value< int >( );
			out = prop_out_.value< int >( );
			step = 1;

			ml::input_type_ptr a = inner_;

			if ( in >= int( a->slot_count( ) ) )
				in = int( a->slot_count( ) ) - 1;
			if ( in < 0 )
				in = int( a->slot_count( ) ) + in;
			if ( in < 0 )
				in = 0;
	
			if ( out >= int( a->slot_count( ) ) )
				out = int( a->slot_count( ) );
			if ( out < 0 )
				out = int( a->slot_count( ) ) + out + 1;
			if ( out < 0 )
				out = 0;
			if ( out < in )
				out --;
	
			if ( out < in )
				step = -1;
		}

		size_t slot_for_position( int position )
		{
			size_t result = 0;
			for ( size_t i = 0; i < slot_count( ); i ++ )
			{
				if ( fetch_slot( i ) )
				{
					result = i;
					if ( position < fetch_slot( i )->get_frames( ) )
						break;
					position -= fetch_slot( i )->get_frames( );
				}
			}
			return result;
		}

		int slot_offset( size_t slot )
		{
			int result = 0;
			for ( size_t i = 0; i < slot; i ++ )
			{
				if ( fetch_slot( i ) )
					result += fetch_slot( i )->get_frames( );
			}
			return result;
		}

		ml::input_type_ptr inner_;
		pcos::property prop_in_;
		pcos::property prop_out_;
		pcos::property prop_original_input_;
		pcos::property prop_serialise_;
		pcos::property prop_serialise_terminator_;
		boost::shared_ptr< pcos::observer > obs_serialise_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_slots( const pl::wstring & )
{
	return ml::filter_type_ptr( new filter_slots( ) );
}

} }
