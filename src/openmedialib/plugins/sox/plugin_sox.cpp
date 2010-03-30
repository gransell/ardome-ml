// sox - A sox plugin to ml.
//
// Copyright (C) 2009 Charles Yates
// Released under the LGPL.

extern "C" {
#include <sox.h>
}

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openpluginlib/pl/pcos/observer.hpp>

#include <boost/algorithm/string.hpp>

#include <iostream>
#include <cstdlib>
#include <vector>
#include <string>
#include <sstream>

namespace pl = olib::openpluginlib;
namespace ml = olib::openmedialib::ml;
namespace il = olib::openimagelib::il;
namespace pcos = olib::openpluginlib::pcos;

namespace olib { namespace openmedialib { namespace ml { namespace sox {

// pcos property observer
template < typename C > class fn_observer : public pcos::observer
{
	public:
		fn_observer( C *instance, void ( C::*fn )( pcos::isubject * ) )
			: instance_( instance )
			, fn_( fn )
		{ }

		virtual void updated( pcos::isubject *subject )
		{ ( instance_->*fn_ )( subject ); }

	private:
		C *instance_;
		void ( C::*fn_ )( pcos::isubject * );
};

static std::string basename( const pl::wstring &spec )
{
	std::string base = pl::to_string( spec );

	// Chop out all the cruft first
	if ( base.find( "sox_doc_" ) == 0 )
		base = base.substr( 8 );

	if ( base.find( "sox_" ) == 0 )
		base = base.substr( 4 );

	return base;
}

class ML_PLUGIN_DECLSPEC filter_sox : public filter_type
{
	public:
		filter_sox( const pl::wstring &spec )
			: filter_type( )
			, spec_( spec )
			, prop_speed_( pl::pcos::key::from_string( "speed" ) )
			, eff_( 0 )
		{
			properties( ).append( prop_speed_ = 1.0 );
		}

		~filter_sox( )
		{
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		virtual const pl::wstring get_uri( ) const { return spec_; }

		virtual const size_t slot_count( ) const { return 1; }

	protected:
		void do_fetch( frame_type_ptr &result )
		{
			result = fetch_from_slot( 0 );

			if ( eff_ == 0 && result->has_audio( ) )
			{
				std::string base = basename( spec_ );
				const sox_effect_handler_t *eff_handle = sox_find_effect( base.c_str( ) );
				eff_ = sox_create_effect( eff_handle );
				enc_.encoding = SOX_ENCODING_SIGN2;
				enc_.bits_per_sample = 32;
				eff_->in_encoding = eff_->out_encoding = &enc_;
				char *temp[ ] = { "2.0", 0 };
				if ( ( * eff_->handler.getopts )( eff_, 1, temp ) == SOX_SUCCESS )
				{
					// Set the sox signal parameters
					eff_->in_signal.rate = result->get_audio( )->frequency( );
					eff_->out_signal.rate = result->get_audio( )->frequency( );
					eff_->in_signal.channels = result->get_audio( )->channels( );
					eff_->out_signal.channels = result->get_audio( )->channels( );
					eff_->in_signal.precision = 32;
					eff_->out_signal.precision = 32;
					eff_->in_signal.length = 0;
					eff_->out_signal.length = 0;
					if ( ( * eff_->handler.start )( eff_ ) != SOX_SUCCESS )
						std::cerr << "this sucks" << std::endl;
				}
			}

			if ( eff_ && result->has_audio( ) )
			{
				ml::audio_type_ptr input = ml::audio::coerce( L"pcm32", result->get_audio( ) );
				ml::audio_type_ptr output = ml::audio::allocate( input, input->frequency( ), input->channels( ), input->samples( ) / 2 );

				size_t isamp = input->samples( );
				size_t osamp = output->samples( );

				std::cerr << "ante: " << isamp << " " << osamp << std::endl;
				if ( ( * eff_->handler.flow )( eff_, static_cast< const sox_sample_t * >( input->pointer( ) ), static_cast< sox_sample_t * >( output->pointer( ) ), &isamp, &osamp ) == SOX_SUCCESS )
				{
					std::cerr << "post: " << isamp << " " << osamp << std::endl;
				}
				else
				{
					std::cerr << "abject failure" << std::endl;
				}
			}
		}

	private:
		pl::wstring spec_;
		pl::pcos::property prop_speed_;
		sox_effect_t *eff_;
		sox_encodinginfo_t enc_;
};

//
// Plugin object
//

class ML_PLUGIN_DECLSPEC plugin : public openmedialib_plugin
{
public:
	virtual input_type_ptr input(  const pl::wstring &spec )
	{
		return input_type_ptr( );
	}

	virtual store_type_ptr store( const pl::wstring &name, const frame_type_ptr &frame )
	{
		return store_type_ptr( );
	}

	virtual filter_type_ptr filter( const pl::wstring &spec )
	{
		return filter_type_ptr( new filter_sox( spec ) );
	}
};

} } } }

//
// Access methods for openpluginlib
//

extern "C"
{
	ML_PLUGIN_DECLSPEC bool openplugin_init( void )
	{
		return true;
	}

	ML_PLUGIN_DECLSPEC bool openplugin_uninit( void )
	{
		return true;
	}
	
	ML_PLUGIN_DECLSPEC bool openplugin_create_plugin( const char*, pl::openplugin** plug )
	{
		*plug = new ml::sox::plugin;
		return true;
	}
	
	ML_PLUGIN_DECLSPEC void openplugin_destroy_plugin( pl::openplugin* plug )
	{ 
		delete static_cast< ml::sox::plugin * >( plug ); 
	}
}
