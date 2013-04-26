// sox - A sox plugin to ml.
//
// Copyright (C) 2009 Charles Yates
// Released under the LGPL.

extern "C" {
#include <sox.h>
}

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openpluginlib/pl/pcos/observer.hpp>

#include <opencorelib/cl/log_defines.hpp>
#include <opencorelib/cl/enforce_defines.hpp>

#include <boost/algorithm/string.hpp>

#include <vector>

namespace pl = olib::openpluginlib;
namespace ml = olib::openmedialib::ml;

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

static std::string basename( const std::wstring &spec )
{
	std::string base = olib::opencorelib::str_util::to_string( spec );

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
	filter_sox( const std::wstring &spec )
	: filter_type( )
	, spec_( spec )
	, prop_speed_( pl::pcos::key::from_string( "speed" ) )
	, eff_( 0 )
	, expected_( 0 )
	, needed_samples_( 0 )
	, source_position_( 0 )
	, last_frame_( )
	{
		properties( ).append( prop_speed_ = 1.0 );
	}
	
	~filter_sox( )
	{
	}
	
	// Indicates if the input will enforce a packet decode
	virtual bool requires_image( ) const { return false; }
	
	virtual const std::wstring get_uri( ) const { return spec_; }
	
	virtual const size_t slot_count( ) const { return 1; }
	
protected:
	void do_fetch( frame_type_ptr &result )
	{
		if( last_frame_ && last_frame_->get_position( ) == get_position( ) )
		{
			result = last_frame_;
			return;
		}
		
		// Create sox effect on first fetch
		if( eff_ == 0 ) 
		{
			initialize_sox( fetch_from_slot( 0 ) );
			reseat_ = ml::audio::create_reseat( );
		}
		
		// If we are not at the expected position drain the effcet and clear the audio reseat
		if ( get_position( ) != expected_ )
		{
			size_t drained = 0;
			std::vector< sox_sample_t > output( needed_samples_ );
			if( ( * eff_->handler.drain )( eff_, &output[ 0 ], &drained ) != SOX_SUCCESS )
				ARLOG_WARN( "Failed draining effect on seek" );
			
			reseat_->clear( );
		}
		
		// We need to fetch the frame for our current position so that we get the correct video if any
		result = fetch_from_slot( 0 );
		
		ml::frame_type_ptr working_frame( result );
		// If there is no audio or no frame then just return that frame
		if( working_frame && working_frame->has_audio( ) )
		{
			// Get our input so that we can seek on it
			ml::input_type_ptr src_input = fetch_slot( 0 );
			
			// Output buffer for sox to write to
			std::vector< sox_sample_t > output( needed_samples_ );

			while( !reseat_->has( needed_samples_ ) )
			{
				// Make sure our source input is at the correct position where we left of
				src_input->seek( source_position_ );
				
				working_frame = src_input->fetch( );
				
				// Sox expects 32 bit samples
				ml::audio_type_ptr input = ml::audio::coerce( L"pcm32", working_frame->get_audio( ) );
				
				size_t isamp = input->samples( );
				size_t osamp = needed_samples_;
				
				if ( ( * eff_->handler.flow )( eff_, static_cast< const sox_sample_t * >( input->pointer( ) ), 
											   static_cast< sox_sample_t * >( &output[ 0 ] ), &isamp, &osamp ) == SOX_SUCCESS )
				{
					// Only append if we have data
					if( osamp != 0 )
					{
						// Lazy right now since the reseat takes audio type
						ml::audio_type_ptr output_audio = ml::audio::allocate( input, input->frequency( ),
																			  input->channels( ), osamp );
						memcpy( output_audio->pointer( ), &output[ 0 ], osamp * sizeof( boost::int32_t ) );
						reseat_->append( output_audio );						
					}
				}
				else
				{
					ARLOG_ERR( "Failed to flow samples from sox effect" );
				}
				
				++source_position_;
			}
			
			result->set_position( get_position( ) );
			// At this point there should be enough audio in the reseat
			result->set_audio( reseat_->retrieve( needed_samples_ ) );
			
			result->set_fps( result->get_fps_num( ) * prop_speed_.value< double >( ), 
							 result->get_fps_den( ) );
		}
		
		last_frame_ = result;
		
		++expected_;
	}
	
	void initialize_sox( const ml::frame_type_ptr &frame )
	{
		ARENFORCE_MSG( frame, "Need a frame to initialize sox" );
		
		std::string base = basename( spec_ );
		const sox_effect_handler_t *eff_handle = sox_find_effect( base.c_str( ) );
		eff_ = sox_create_effect( eff_handle );
		enc_.encoding = SOX_ENCODING_SIGN2;
		enc_.bits_per_sample = 32;
		eff_->in_encoding = eff_->out_encoding = &enc_;
		const char *temp[ ] = { "tempo", "2", 0 };
		if ( ( * eff_->handler.getopts )( eff_, 2, const_cast< char ** >( temp ) ) == SOX_SUCCESS )
		{
			// Set the sox signal parameters
			eff_->in_signal.rate = frame->get_audio( )->frequency( );
			eff_->out_signal.rate = frame->get_audio( )->frequency( );
			eff_->in_signal.channels = frame->get_audio( )->channels( );
			eff_->out_signal.channels = frame->get_audio( )->channels( );
			eff_->in_signal.precision = 32;
			eff_->out_signal.precision = 32;
			eff_->in_signal.length = 0;
			eff_->out_signal.length = 0;
			ARENFORCE_MSG( ( * eff_->handler.start )( eff_ ) == SOX_SUCCESS, "Failed initializing sox." );
		}
		
		needed_samples_ = (double)frame->get_audio( )->samples( ) / prop_speed_.value< double >( );
	}
	
private:
	std::wstring spec_;
	pl::pcos::property prop_speed_;
	sox_effect_t *eff_;
	sox_encodinginfo_t enc_;
	ml::audio::reseat_ptr reseat_;
	boost::int32_t expected_;
	boost::int32_t needed_samples_;
	boost::int32_t source_position_;
	ml::frame_type_ptr last_frame_;
};

//
// Plugin object
//

class ML_PLUGIN_DECLSPEC plugin : public openmedialib_plugin
{
public:
	virtual input_type_ptr input(  const std::wstring &spec )
	{
		return input_type_ptr( );
	}

	virtual store_type_ptr store( const std::wstring &name, const frame_type_ptr &frame )
	{
		return store_type_ptr( );
	}

	virtual filter_type_ptr filter( const std::wstring &spec )
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
