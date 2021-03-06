// AWI Input handler - generates an input from a ts_auto generated awi file
//
// Copyright (C) 2009 Ardendo
// Released under the LGPL.
//
// #input:awi:
//
// Allows direct playback of indexed files.

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"
#include <cmath>

namespace aml { namespace openmedialib {

class ML_PLUGIN_DECLSPEC input_awi : public ml::input_type
{
	public:
		input_awi( const std::wstring &resource ) 
			: ml::input_type( )
			, resource_( resource )
			, prop_video_index_( pcos::key::from_string( "video_index" ) )
			, prop_audio_index_( pcos::key::from_string( "audio_index" ) )
			, internal_( )
		{
			properties( ).append( prop_video_index_ = 0 );
			properties( ).append( prop_audio_index_ = 0 );
		}

		virtual ~input_awi( ) { }

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		// Basic information
		virtual const std::wstring get_uri( ) const { return resource_; }
		virtual const std::wstring get_mime_type( ) const { return L""; }

		// Audio/Visual
		virtual int get_frames( ) const { return internal_ ? internal_->get_frames( ) : 0; }
		virtual bool is_seekable( ) const { return true; }

	protected:
		virtual bool initialize( )
		{
			std::wstring uri = resource_.substr( 0, resource_.find( L".awi" ) );

			if ( prop_video_index_.value< int >( ) != -1 && prop_audio_index_.value< int >( ) != -1 )
			{
				ml::input_type_ptr video = ml::create_delayed_input( uri );
				ml::input_type_ptr audio = ml::create_delayed_input( uri );

				if ( video && audio )
				{
					video->property( "video_index" ) = prop_video_index_.value< int >( );
					video->property( "audio_index" ) = -1;
					video->property( "ts_auto" ) = 1;

					audio->property( "video_index" ) = -1;
					audio->property( "audio_index" ) = prop_audio_index_.value< int >( );
					audio->property( "ts_auto" ) = 1;

					if ( video->init( ) )
					{
						ml::frame_type_ptr sample = video->fetch( );
						int num = 25, den = 1;
						if ( sample && sample->has_image( ) )
							sample->get_fps( num, den );
						audio->property( "fps_num" ) = num;
						audio->property( "fps_den" ) = den;
					}

					if ( audio->init( ) )
					{
						ml::filter_type_ptr video_avdecode = ml::create_filter( L"avdecode" );
						ml::filter_type_ptr audio_avdecode = ml::create_filter( L"avdecode" );
						ml::filter_type_ptr mvitc = ml::create_filter( L"mvitc_decode" );
						ml::filter_type_ptr muxer = ml::create_filter( L"muxer" );
						video_avdecode->connect( video );
						mvitc->connect( video_avdecode );
						muxer->connect( mvitc, 0 );
						audio_avdecode->connect( audio );
						muxer->connect( audio_avdecode, 1 );
						internal_ = muxer;
					}
				}
			}
			else if ( prop_video_index_.value< int >( ) != -1 )
			{
				ml::input_type_ptr input = ml::create_delayed_input( uri );
				if ( input )
				{
					input->property( "video_index" ) = prop_video_index_.value< int >( );
					input->property( "audio_index" ) = prop_audio_index_.value< int >( );
					input->property( "ts_auto" ) = 1;

					if ( input->init( ) )
					{
						ml::filter_type_ptr avdecode = ml::create_filter( L"avdecode" );
						ml::filter_type_ptr mvitc = ml::create_filter( L"mvitc_decode" );
						avdecode->connect( input );
						mvitc->connect( avdecode );
						internal_ = mvitc;
					}
				}
			}
			else if ( prop_audio_index_.value< int >( ) != -1 )
			{
				ml::input_type_ptr input = ml::create_delayed_input( uri );
				if ( input )
				{
					input->property( "video_index" ) = 0;
					input->property( "audio_index" ) = -1;
					input->property( "ts_auto" ) = 1;

					if ( input->init( ) )
					{
						int num = 25, den = 1;

						ml::frame_type_ptr sample = input->fetch( );

						if ( sample && sample->get_image( ) )
							sample->get_fps( num, den );

						internal_ = ml::create_delayed_input( uri );

						internal_->property( "video_index" ) = -1;
						internal_->property( "audio_index" ) = prop_audio_index_.value< int >( );
						internal_->property( "fps_num" ) = num;
						internal_->property( "fps_den" ) = den;
						internal_->property( "ts_auto" ) = 1;

						internal_->init( );
					}
				}
			}

			return internal_;
		}

		// Fetch method
		void do_fetch( ml::frame_type_ptr &result )
		{
			if ( internal_ )
			{
				internal_->seek( get_position( ) );
				result = internal_->fetch( );
			}
		}

		void sync_frames( )
		{
			if ( internal_ )
				internal_->sync( );
		}

	private:
		std::wstring resource_;
		pcos::property prop_video_index_;
		pcos::property prop_audio_index_;
		ml::input_type_ptr internal_;
};

ml::input_type_ptr ML_PLUGIN_DECLSPEC create_input_awi( const std::wstring &resource )
{
	return ml::input_type_ptr( new input_awi( resource ) );
}

} }
