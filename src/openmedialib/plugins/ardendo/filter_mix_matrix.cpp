// Locked Audio Filter
//
// Copyright (C) 2009 Ardendo
// Released under the terms of the LGPL.
//
// #filter:mix_matrix
//
// Provides a mix matrix filter. For example, to convert a 4 channel input to stereo:
//
// tone: channels=4 filter:mix_matrix matrix="1 0  0 1  1 0  0 1" channels=2
//
// The matrix has as many rows as there are input channels, and as many columns as there
// are output channels - input is mixed accordingly.

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"

#include <openpluginlib/pl/log.hpp>

#include <opencorelib/cl/core.hpp>
#include <opencorelib/cl/log_defines.hpp>

namespace aml { namespace openmedialib {

class ML_PLUGIN_DECLSPEC filter_mix_matrix : public ml::filter_simple
{
	public:
		// Filter_type overloads
		explicit filter_mix_matrix( const pl::wstring & )
			: ml::filter_simple( )
			, prop_enable_( pcos::key::from_string( "enable" ) )
			, prop_channels_( pcos::key::from_string( "channels" ) )
			, prop_matrix_( pcos::key::from_string( "matrix" ) )
			, matrix_observer( new fn_observer< filter_mix_matrix >( this, &filter_mix_matrix::matrix_changed ) )
		{
			properties( ).append( prop_enable_ = 1 );
			properties( ).append( prop_channels_ = 2 );
			properties( ).append( prop_matrix_ = pl::wstring( L"1 0 0 1" ) );

			prop_matrix_.attach( matrix_observer );
		}

		//Called whenever the matrix property are updated
		void matrix_changed( )
		{
			//Parse the mix matrix string property and store it in the matrix_ vector
			matrix_.clear( );

			pl::wstring matrix = prop_matrix_.value< pl::wstring >( );
			std::stringstream stream;
			stream << pl::to_string( matrix );
			while ( !stream.eof( ) )
			{
				double v;
				stream >> v;
				if( !stream.fail() )
					matrix_.push_back( v );
			}
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		// This provides the name of the plugin (used in serialisation)
		virtual const pl::wstring get_uri( ) const { return L"mix_matrix"; }
		
	protected:
		// The main access point to the filter
		void do_fetch( ml::frame_type_ptr &result )
		{
			result = fetch_from_slot( );
			if ( prop_enable_.value< int >( ) )
			{
				ml::audio_type_ptr audio = result->get_audio( );
				result->set_audio( ml::audio::mix_matrix( audio, matrix_, prop_channels_.value< int >( ) ) );
			}
		}

		pcos::property prop_enable_;
		pcos::property prop_channels_;
		pcos::property prop_matrix_;
		boost::shared_ptr< pl::pcos::observer > matrix_observer;
		std::vector< double > matrix_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_mix_matrix( const pl::wstring &resource )
{
	return ml::filter_type_ptr( new filter_mix_matrix( resource ) );
}

} }
