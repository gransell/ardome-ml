// Volume filter
//
// Copyright (C) 2007 Ardendo
// Released under the terms of the LGPL.
//
// #filter:lowpass
//
// This is an implementation of a Chebyshev Type 1 lowpass filter.

#include "precompiled_headers.hpp"
#include <limits>
#include <cmath>
#include <iostream>
#include "amf_filter_plugin.hpp"
#include "utility.hpp"

namespace aml { namespace openmedialib {

class ML_PLUGIN_DECLSPEC filter_lowpass : public ml::filter_simple
{
	public:
		// Filter_type overloads
		explicit filter_lowpass( const pl::wstring & )
			: ml::filter_simple( )
			, prop_mag_( pcos::key::from_string( "mag" ) )
		{
		  properties( ).append( prop_mag_ = 14 );
			size = -1;
		}

		// This provides the name of the plugin (used in serialisation)
		virtual const pl::wstring get_uri( ) const { return L"lowpass"; }

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

	protected:

		// The main access point to the filter
		void do_fetch( ml::frame_type_ptr &result )
		{
		  result = fetch_from_slot( );
		  
		  ml::audio_type_ptr input_audio  = ml::audio::force( ml::audio::FORMAT_FLOAT, result->get_audio( ) );
 		  ml::audio_type_ptr output_audio = ml::audio::force( ml::audio::FORMAT_FLOAT, result->get_audio( ) );
		  int samples                     =          input_audio->samples( );
		  int channels                    =          input_audio->channels( );
		  float *input_ptr                = (float*) input_audio->pointer( );
		  float *output_ptr               = (float*) output_audio->pointer( );
		  int mag                         = prop_mag_.value< int >( );

		  if(mag<1){
		    result->set_audio( input_audio );
		    return;
		  }

		  if(size == -1){
		    size  = channels*prop_mag_.value< int >( );
		    x_n.resize(size);
		  }
		  for ( int i = 0; i < samples; i++ )
		    for ( int j = 0; j < channels; j++)
		      {
			float x_0 = *input_ptr;
			for( int n=j; n<size; n=n+channels)
			  x_0 = x_0+x_n[n];
			*output_ptr = x_0/(size/channels);
			for( int n=(channels*(mag-1))+j; n>channels; n=n-channels){
			  x_n[n] = x_n[n-channels];
			}
			x_n[j]=*input_ptr;
			
			input_ptr++;
			output_ptr++;
		      }
		  result->set_audio( output_audio );
		}
		pcos::property prop_mag_;
		std::vector<float> x_n;
		int size;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_pass( const pl::wstring &resource )
{
	return ml::filter_type_ptr( new filter_lowpass( resource ) );
}

} }
