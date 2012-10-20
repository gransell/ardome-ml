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
		explicit filter_lowpass( const std::wstring & )
			: ml::filter_simple( )
			, prop_mag_( pcos::key::from_string( "mag" ) )
		{
		  properties( ).append( prop_mag_ = 14 );
			size = -1;
		}

		// This provides the name of the plugin (used in serialisation)
		virtual const std::wstring get_uri( ) const { return L"lowpass"; }

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
		  int mag = prop_mag_.value< int >( );
		  if(mag <= 1){
		    result->set_audio( input_audio );
		    return;
		  }

		  if(size == -1){
		    size = prop_mag_.value< int >( );
		    for(int i = 0; i < channels; i++){
		      std::vector<float> p;
		      p.resize(size);
		      x_n.push_back(p);
		    }
		  }
		  for ( int i = 0; i < samples; i++ )
		    for ( int j = 0; j < channels; j++)
		      {
			float x_0 = *input_ptr;
			for( int n=0; n<1; n++)
			  x_0 = x_0+(x_n[j][n]*0.8);
			*output_ptr = x_0*0.625;
			x_n[j].erase(x_n[j].begin());
			x_n[j].push_back(*input_ptr);
			input_ptr++;
			output_ptr++;
		      }
		  result->set_audio( output_audio );
		}
		pcos::property prop_mag_;
		std::vector< std::vector<float> > x_n;
		int size;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_pass( const std::wstring &resource )
{
	return ml::filter_type_ptr( new filter_lowpass( resource ) );
}

} }
