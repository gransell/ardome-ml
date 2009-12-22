// Stream packet decoder and encoder filters
//
// Copyright (C) 2009 Ardendo
// Released under the terms of the LGPL.
//
// #filter:avdecode
//
// Provides stream/packet based decoding. This is typically used for non-avformat:
// input implementations to provide a means to decode the packets retrieved.
// 
// #filter:avencode
// 
// Provides stream packet encoding.

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/packet.hpp>
#include <openpluginlib/pl/pcos/isubject.hpp>
#include <openpluginlib/pl/pcos/observer.hpp>
#include <openmedialib/ml/scope_handler.hpp>

#include <vector>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include "avformat_stream.hpp"

namespace ml = olib::openmedialib::ml;
namespace pl = olib::openpluginlib;
namespace il = olib::openimagelib::il;

namespace olib { namespace openmedialib { namespace ml {

extern std::map< CodecID, std::string > codec_name_lookup_;
extern std::map< std::string, CodecID > name_codec_lookup_;

extern const std::wstring avformat_to_oil( int );
extern const PixelFormat oil_to_avformat( const std::wstring & );
extern il::image_type_ptr convert_to_oil( AVFrame *, PixelFormat, int, int );

static const pl::pcos::key key_gop_closed_ = pl::pcos::key::from_string( "gop_closed" );

static bool is_dv( const std::string &codec )
{
    return codec == "dv" || codec == "dv25" || codec == "dv50" || codec == "dvcprohd_1080i";
}

static bool is_mpeg2( const std::string &codec )
{
    return codec == "mpeg2" || codec == "mpeg2/mpeg2hd_1080i";
}

static bool is_imx( const std::string &codec )
{
    return codec == "mpeg2/30" || codec == "mpeg2/50";
}

class stream_queue
{
    public:
        stream_queue( ml::input_type_ptr input, int gop_open, const std::wstring& scope )
            : input_( input )
            , gop_open_( gop_open )
            , keys_( 0 )
            , context_( 0 )
            , codec_( 0 )
            , expected_( 0 )
            , frame_( avcodec_alloc_frame( ) )
            , offset_( 0 )
        {
            audio_buf_size_ = (AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2;
            audio_buf_ = ( uint8_t * )av_malloc( audio_buf_size_ );
        }

        virtual ~stream_queue( )
        {
            if ( context_ )
                avcodec_close( context_ );
            av_free( frame_ );
        }

        boost::uint8_t *find_mpeg2_gop( const ml::stream_type_ptr &stream )
        {
            boost::uint8_t *result = 0;
            boost::uint8_t *ptr = stream->bytes( );
            unsigned int index = 0;
            unsigned int length = stream->length( );
            while ( result == 0 && index + 10 < length )
            {
                if ( ptr[ index ] == 0 && ptr[ index + 1 ] == 0 && ptr[ index + 2 ] == 1 )
                {
                    if ( ptr[ index + 3 ] == 0xb8 )
                    {
                        if ( ptr[ index + 8 ] == 0 && ptr[ index + 9 ] == 0 && ptr[ index + 10 ] == 1 )
                            result = ptr + index;
                        else
                            index += 4;
                    }
                    else if ( ptr[ index + 3 ] == 0x00 )
                        index += 3;
                    else
                        index += 4;
                }
                else
                {
                    index ++;
                }
                if ( !result && ptr[ index ] != 0x00 )
                {
                    boost::uint8_t *t = ( boost::uint8_t * )memchr( ptr + index, 0x00, length - index );
                    if ( t == 0 ) break;
                    index = t - ptr;
                }
            }
    
            return result;
        }

        void look_for_closed( const ml::frame_type_ptr &frame )
        {
            pl::pcos::property gop_closed( key_gop_closed_ );
            if ( is_dv( frame->get_stream( )->codec( ) ) || is_imx( frame->get_stream( )->codec( ) ) )
            {
                frame->get_stream( )->properties( ).append( gop_closed = 1 );
            }
            else if ( is_mpeg2( frame->get_stream( )->codec( ) ) )
            {
                boost::uint8_t *ptr = find_mpeg2_gop( frame->get_stream( ) );
                if ( ptr ) frame->get_stream( )->properties( ).append( gop_closed = int( ( ptr[ 7 ] & 64 ) >> 6 ) );
            }
            else
            {
                frame->get_stream( )->properties( ).append( gop_closed = -1 );
            }
        }

        ml::frame_type_ptr fetch( int position )
        {
            ml::frame_type_ptr result;
            
            lru_cache_type_ptr lru_cache = ml::the_scope_handler::Instance().lru_cache( scope_ );
            lru_cache_type::key_type my_key( position, input_->get_uri() );
                        
            result = lru_cache->frame_for_position( my_key );
            
            if( !result && position < input_->get_frames( ) )
            {
                //std::cerr << "fetching from input " << position << std::endl;
                input_->seek( position );
                result = input_->fetch( );
                while ( result && result->get_stream( ) )
                {
                    //std::cerr << "obtained from input " << result->get_position( ) << " for " << position << std::endl;
                    if ( result->get_stream( )->position( ) == result->get_stream( )->key( ) )
                        look_for_closed( result );
                        
                    lru_cache->insert_frame_for_position( my_key, result );
                    
                    if ( result->get_position( ) == position )
                        break;
                    input_->seek( result->get_position( ) + 1 );
                    result = input_->fetch( );
                }
            }

            return result;
        }

        il::image_type_ptr decode_image( int position )
        {
            lru_cache_type_ptr lru_cache = ml::the_scope_handler::Instance().lru_cache( scope_ );
            lru_cache_type::key_type my_key( position, input_->get_uri() );
                        
            il::image_type_ptr img = lru_cache->image_for_position( my_key );
            
            if( img )
            {
                return img;
            }
        
            ml::frame_type_ptr frame = fetch( position );

            if ( frame && frame->get_stream( ) )
            {
                int start = expected_;

                if ( frame->get_stream( )->codec( ).find( "dv" ) != 0 )
                {
                    if ( position + offset_ != expected_ )
                        start = frame->get_stream( )->key( );

                    if ( start == position && position != 0 )
                        start = fetch( start - 1 )->get_stream( )->key( );
                }
                else
                {
                    start = position;
                }

                ml::frame_type_ptr temp;

                if ( start < input_->get_frames( ) )
                {
                    temp = ml::frame_type_ptr( new frame_type( fetch( start ++ ) ) );

                    while( temp && temp->get_stream( ) && decode( temp, position ) )
                    {
                        if ( start < input_->get_frames( ) )
                            temp = ml::frame_type_ptr( new frame_type( fetch( start ++ ) ) );
                        else
                            temp = ml::frame_type_ptr( );
                    }
                }

                if ( temp )
                {
                    return temp->get_image( );
                }
                else if ( frame->get_stream( )->codec( ).find( "dv" ) != 0 )
                {
                    int got = 0;
                    if ( avcodec_decode_video( context_, frame_, &got, 0, 0 ) >= 0 )
                    {
                        il::image_type_ptr image;
                        if ( got )
                        {
                            const PixelFormat fmt = context_->pix_fmt;
                            const int width = context_->width;
                            const int height = context_->height;
                            image = convert_to_oil( frame_, fmt, width, height );
                            image->set_position( input_->get_frames( ) - 1 );
                            if ( frame_->interlaced_frame )
                                image->set_field_order( frame_->top_field_first ? il::top_field_first : il::bottom_field_first );
                                
                            lru_cache->insert_image_for_position( lru_cache_type::key_type( image->position(), input_->get_uri() ), image );
                        }
                        expected_ ++;
                        return image;
                    }
                }
            }

            return il::image_type_ptr( );
        }

        ml::audio_type_ptr decode_audio( int position )
        {
            lru_cache_type_ptr lru_cache = ml::the_scope_handler::Instance().lru_cache( scope_ );
            lru_cache_type::key_type my_key( position, input_->get_uri() );
            
            audio_type_ptr aud = lru_cache->audio_for_position( my_key );
            
            if( aud )
                return aud;
            
            ml::frame_type_ptr frame = fetch( position );

            if ( frame && frame->get_stream( ) )
            {
                int start = expected_;

                if ( position + offset_ != expected_ )
                    start = frame->get_stream( )->key( );

                if ( gop_open_ && start == position && position != 0 )
                    start = fetch( start - 1 )->get_stream( )->key( );

                ml::frame_type_ptr temp = ml::frame_type_ptr( new frame_type( fetch( start ++ ) ) );

                while( temp && temp->get_stream( ) && decode( temp, position ) )
                    temp = ml::frame_type_ptr( new frame_type( fetch( start ++ ) ) );

                return temp->get_audio( );
            }

            return ml::audio_type_ptr( );
        }

        ml::fraction sar( )
        {
            ml::fraction result( 0, 1 );
            if ( context_ )
            {
                result.num = context_->sample_aspect_ratio.num;
                result.den = context_->sample_aspect_ratio.den;
            }
            return result;
        }

        bool decode( ml::frame_type_ptr &result, int position )
        {
            bool found = false;

            if ( context_ == 0 )
            {
                context_ = avcodec_alloc_context( );
                context_->thread_count = 4;
                codec_ = avcodec_find_decoder( name_codec_lookup_[ result->get_stream( )->codec( ) ] );
                avcodec_open( context_, codec_ );
                avcodec_thread_init( context_, 4 );
                avcodec_flush_buffers( context_ );
            }

            ml::stream_type_ptr pkt = result->get_stream( );

            if ( result->get_position( ) != expected_ )
            {
                if ( pkt->id( ) == ml::stream_video )
                    avcodec_flush_buffers( context_ );
                expected_ = result->get_position( );
            }

            int got = 0;
            int audio_size = audio_buf_size_;

            switch( pkt->id( ) )
            {
                case ml::stream_video:
                    if ( avcodec_decode_video( context_, frame_, &got, pkt->bytes( ), pkt->length( ) ) >= 0 )
                    {
                        if ( got )
                        {
                            il::image_type_ptr image;
                            const PixelFormat fmt = context_->pix_fmt;
                            const int width = context_->width;
                            const int height = context_->height;
                            image = convert_to_oil( frame_, fmt, width, height );
                            image->set_position( pkt->position( ) );
                            result->set_image( image );

                            if ( frame_->interlaced_frame )
                                image->set_field_order( frame_->top_field_first ? il::top_field_first : il::bottom_field_first );

                            if ( position == 0 )
                            {
                                //std::cerr << "found first " << result->get_position( ) << " " << expected_ << std::endl;
                                offset_ = result->get_position( );
                            }

                            image->set_position( result->get_position( ) - offset_ );
                            lru_cache_type_ptr lru_cache = ml::the_scope_handler::Instance().lru_cache( scope_ );
                            lru_cache->insert_image_for_position( lru_cache_type::key_type( image->position( ), input_->get_uri( ) ), image );

                            if ( result->get_position( ) >= position + offset_ )
                                found = true;
                        }
                        else
                        {
                            //std::cerr << "no image for " << result->get_position( ) << std::endl;
                            offset_ = 1;
                        }
    
                        expected_ ++;
                    }

                    break;

                case ml::stream_audio:
                    if ( avcodec_decode_audio2( context_, ( short * )( audio_buf_ ), &audio_size, pkt->bytes( ), pkt->length( ) ) >= 0 )
                    {
                        int channels = context_->channels;
                        int frequency = context_->sample_rate;

                        audio::pcm16_ptr audio = audio::pcm16_ptr( new audio::pcm16( frequency, channels, audio_size / channels / 2 ) );
                        memcpy( audio->pointer( ), audio_buf_, audio->size( ) );
                        audio->set_position( pkt->position( ) );
                        result->set_audio( audio );

                        if ( result->get_position( ) >= position + offset_ )
                            found = true;

                        expected_ ++;
                    }
                    break;

                default:
                    break;
            }

            return !found;
        }

    private:
        ml::input_type_ptr input_;
        int gop_open_;
        std::wstring scope_;

        int keys_;
        AVCodecContext *context_;
        AVCodec *codec_;
        int expected_;
        AVFrame *frame_;
        int audio_buf_size_;
        boost::uint8_t *audio_buf_;
        int offset_;
};

typedef boost::shared_ptr< stream_queue > stream_queue_ptr;

class ML_PLUGIN_DECLSPEC frame_avformat : public ml::frame_type 
{
    public:
        /// Constructor
        frame_avformat( const frame_type_ptr &other, const stream_queue_ptr &queue )
            : ml::frame_type( *other )
            , queue_( queue )
        {
        }

        /// Destructor
        virtual ~frame_avformat( )
        {
        }

        /// Provide a shallow copy of the frame (and all attached frames)
        virtual frame_type_ptr shallow( )
        {
            return frame_type_ptr( new frame_avformat( frame_type::shallow( ), queue_ ) );
        }

        /// Provide a deepy copy of the frame (and a shallow copy of all attached frames)
        virtual frame_type_ptr deep( )
        {
            return frame_type_ptr( new frame_avformat( frame_type::deep( ), queue_ ) );
        }

        /// Indicates if the frame has an image
        virtual bool has_image( )
        {
            return image_ || ( stream_ && stream_->id( ) == ml::stream_video );
        }

        /// Indicates if the frame has audio
        virtual bool has_audio( )
        {
            return audio_ || ( stream_ && stream_->id( ) == ml::stream_audio );
        }

        /// Set the image associated to the frame.
        virtual void set_image( olib::openimagelib::il::image_type_ptr image )
        {
            image_ = image;
            //Destroy the existing video stream, since it is not a correct
            //representation of the image anymore
            if( stream_ && stream_->id() == stream_video )
            {
                stream_.reset();
            }
        }

        /// Get the image associated to the frame.
        virtual olib::openimagelib::il::image_type_ptr get_image( )
        {
            if ( !image_ && ( stream_ && stream_->id( ) == ml::stream_video ) )
            {
                image_ = queue_->decode_image( stream_->position( ) );
                ml::fraction sar = queue_->sar( );
                if ( sar.num && sar.den )
                    set_sar( sar.num, sar.den );
            }
            return image_;
        }

        /// Set the audio associated to the frame.
        virtual void set_audio( audio_type_ptr audio )
        {
            audio_ = audio;
            //Destroy the existing audio stream, since it is not a correct
            //representation of the audio anymore
            if( stream_ && stream_->id() == stream_audio )
            {
                stream_.reset();
            }
        }

        /// Get the audio associated to the frame.
        virtual audio_type_ptr get_audio( )
        {
            if ( !audio_ && ( stream_ && stream_->id( ) == ml::stream_audio ) )
                audio_ = queue_->decode_audio( stream_->position( ) );
            return audio_;
        }

        // Calculate the duration of the frame
        virtual double get_duration( ) const
        {
            int num, den;
            get_fps( num, den );
            return double( den ) / double( num );
        }

    private:
        stream_queue_ptr queue_;
};

class avformat_decode_filter : public filter_type
{
    public:
        // Filter_type overloads
        avformat_decode_filter( )
            : ml::filter_type( )
            , prop_gop_open_( pl::pcos::key::from_string( "gop_open" ) )
            , prop_scope_( pl::pcos::key::from_string( "scope" ) )
            , initialised_( false )
            , queue_( )
        {
            properties( ).append( prop_gop_open_ = 1 );
            properties( ).append( prop_scope_ = pl::wstring(L"default_scope") );
        }

        virtual ~avformat_decode_filter( )
        {
        }

        // Indicates if the input will enforce a packet decode
        virtual bool requires_image( ) const { return false; }

        // This provides the name of the plugin (used in serialisation)
        virtual const pl::wstring get_uri( ) const { return L"avdecode"; }

        virtual int get_frames( ) const
        {
            if ( fetch_slot( 0 ) ) 
                return fetch_slot( 0 )->get_frames( );
            return 0;
        }

    protected:
        
        // The main access point to the filter
        void do_fetch( ml::frame_type_ptr &result )
        {
            if ( !initialised_ )
            {
                result = fetch_from_slot( 0 );

                if ( result && result->get_stream( ) )
                {
                    queue_ = stream_queue_ptr( new stream_queue( fetch_slot( 0 ), prop_gop_open_.value< int >( ), prop_scope_.value<pl::wstring>() ) );
                    initialised_ = true;
                }
            }

            if ( queue_ )
            {
                result = queue_->fetch( get_position( ) );
                if ( result )
                    result = ml::frame_type_ptr( new frame_avformat( result, queue_ ) );
            }
            else
            {
                result = fetch_from_slot( 0 );
            }

            // Make sure all frames are shallow copied here
            result = result->shallow( );
        }

    private:
        pl::pcos::property prop_gop_open_;
        pl::pcos::property prop_scope_;
        bool initialised_;
        stream_queue_ptr queue_;
};

class avformat_encode_filter : public filter_type
{
    public:
        // Filter_type overloads
        avformat_encode_filter( )
            : ml::filter_type( )
            , prop_enable_( pl::pcos::key::from_string( "enable" ) )
            , prop_force_( pl::pcos::key::from_string( "force" ) )
            , prop_codec_( pl::pcos::key::from_string( "codec" ) )
            , initialised_( false )
            , encoding_( false )
            , context_( 0 )
            , instance_( 0 )
            , key_( 0 )
            , pf_( L"" )
        {
            properties( ).append( prop_enable_ = 1 );
            properties( ).append( prop_force_ = 0 );
            properties( ).append( prop_codec_ = pl::wstring( L"mpeg2" ) );
        }

        virtual ~avformat_encode_filter( )
        {
        }

        // Indicates if the input will enforce a packet decode
        virtual bool requires_image( ) const { return false; }

        // This provides the name of the plugin (used in serialisation)
        virtual const pl::wstring get_uri( ) const { return L"avencode"; }

        virtual int get_frames( ) const
        {
            if ( fetch_slot( 0 ) ) 
                return fetch_slot( 0 )->get_frames( );
            return 0;
        }

    protected:
        // The main access point to the filter
        void do_fetch( ml::frame_type_ptr &result )
        {
            if ( prop_enable_.value< int >( ) )
            {
                ml::frame_type_ptr source = fetch_from_slot( 0 );

                if ( !initialised_ )
                {
                    result = source;
    
                    // Try to set up an internal graph to handle the cpu compositing of deferred frames
                    pusher_ = ml::create_input( L"pusher:" );
                    render_ = ml::create_filter( L"render" );
    
                    if ( render_ )
                        render_->connect( pusher_ );
    
                    initialised_ = true;
                    pf_ = result->pf( );
                }
    
                if ( !last_frame_ || get_position( ) != last_frame_->get_position( ) )
                {
                    int gop_closed = 0;

                    if ( source->get_stream( ) && source->get_stream( )->properties( ).get_property_with_key( key_gop_closed_ ).valid( ) )
                    {
                        if ( source->get_stream( )->properties( ).get_property_with_key( key_gop_closed_ ).value< int >( ) == 1 )
                            gop_closed = 1;
                    }
    
                    if ( prop_force_.value< int >( ) )
                    {
                        encoding_ = true;
                    }
                    else if ( !source->get_stream( ) )
                    {
                        std::cerr << "case 0: " << get_position( ) << " no stream component" << std::endl;
                        encoding_ = true;
                    }
                    else if ( source->is_deferred( ) )
                    {
                        std::cerr << "case 1: " << get_position( ) << " deferred frame must be rendered" << std::endl;
                        encoding_ = true;
                    }
                    else if ( !last_frame_ && source->get_stream( )->key( ) != source->get_stream( )->position( ) )
                    {
                        std::cerr << "case 2: " << get_position( ) << " first frame not an i frame" << std::endl;
                        encoding_ = true;
                    }
                    else if ( !encoding_ && source->get_stream( )->codec( ) != pl::to_string( prop_codec_.value< pl::wstring >( ) ) )
                    {
                        std::cerr << "case 3: " << get_position( ) << " stream of different type: " << source->get_stream( )->codec( ) << " vs. " << pl::to_string( prop_codec_.value< pl::wstring >( ) ) << std::endl;
                        encoding_ = true;
                    }
                    else if ( !encoding_ && !continuous( last_frame_, source ) )
                    {
                        std::cerr << "case 4: " << get_position( ) << " started encoding due to non-contiguous packets" << std::endl;
                        encoding_ = true;
                    }
                    else if ( encoding_ && ( source->get_stream( )->codec( ) != pl::to_string( prop_codec_.value< pl::wstring >( ) ) || source->get_stream( )->key( ) != source->get_stream( )->position( ) ) )
                    {
                        std::cerr << "case 5: " << get_position( ) << " already encoding and stream component does not indicate that we have reached the next key frame yet" << std::endl;
                    }
                    else if ( encoding_ && source->get_stream( )->key( ) == source->get_stream( )->position( ) && gop_closed )
                    {
                        std::cerr << "case 6: " << get_position( ) << " encoding turned off as next key is reached" << std::endl;
                        encoding_ = false;
                        avcodec_flush_buffers( context_ );
                    }
    
                    if ( encoding_ )
                    {
                        result = render( source );
                        result->set_position( source->get_position( ) );
                        if ( result->get_image( ) )
                            encode( result );
                        else
                            std::cerr << "Stream: no image after render" << std::endl;
                    }
                    else
                    {
                        result = source;
                    }
    
                    last_frame_ = result;
                }
                else
                {
                    result = last_frame_;
                }
            }
            else
            {
                result = fetch_from_slot( 0 );
            }
        }
    
        ml::frame_type_ptr render( const ml::frame_type_ptr &frame )
        {
            ml::frame_type_ptr result = frame;

            if ( render_ )
            {
                pusher_->push( frame );
                result = render_->fetch( );
            }
            else
            {
                result->get_image( );
            }

            result = ml::frame_convert( result, pf_ );

            return result;
        }

        bool continuous( const ml::frame_type_ptr &last, const ml::frame_type_ptr &current )
        {
            bool result = true;

            // Check for continuity
            // TODO: ensure that the source hasn't changed
            if ( last && last->get_stream( ) )
            {
                result = last->get_stream( )->position( ) == current->get_stream( )->position( ) - 1;
                if ( !result )
                    result = current->get_stream( )->position( ) == current->get_stream( )->key( );
            }
            else
            {
                result = current->get_stream( )->position( ) == current->get_stream( )->key( );
            }

            return result;
        }

        void encode( const ml::frame_type_ptr &result )
        {
            if ( context_ == 0 )
            {
                context_ = avcodec_alloc_context( );
                picture_ = avcodec_alloc_frame( );

                if ( is_mpeg2( pl::to_string( prop_codec_.value< pl::wstring >( ) ) ) )
                {
                    instance_ = avcodec_find_encoder( CODEC_ID_MPEG2VIDEO );
                    context_->qmin = 1;
                    context_->lmin = FF_QP2LAMBDA;
                    context_->bit_rate = 50000000;
                    context_->rc_max_available_vbv_use = 1.0;
                    context_->rc_min_vbv_overflow_use = 1.0;
                    context_->rc_min_rate = context_->bit_rate;
                    context_->rc_max_rate = context_->bit_rate;
                    context_->rc_buffer_size = 36408333;
                    context_->gop_size = 12;
                    context_->max_b_frames = 0;
                    context_->pix_fmt = oil_to_avformat( result->pf( ) );
                    context_->flags |= CODEC_FLAG_CLOSED_GOP | CODEC_FLAG_INTERLACED_ME | CODEC_FLAG_INTERLACED_DCT;
                    context_->flags2 |= CODEC_FLAG2_INTRA_VLC;
                    context_->scenechange_threshold = 1000000000;
                    context_->intra_dc_precision = 1;
                    context_->thread_count = 4;
                }
                else if ( is_imx( pl::to_string( prop_codec_.value< pl::wstring >( ) ) ) )
                {
                    instance_ = avcodec_find_encoder( CODEC_ID_MPEG2VIDEO );
                    context_->qmin = 1;
                    context_->lmin = FF_QP2LAMBDA;
                    context_->bit_rate = 50000;
                    context_->rc_max_available_vbv_use = 1.0;
                    context_->rc_min_vbv_overflow_use = 1.0;
                    context_->rc_min_rate = context_->bit_rate;
                    context_->rc_max_rate = context_->bit_rate;
                    context_->rc_buffer_size = 36408333;
                    context_->gop_size = 1;
                    context_->max_b_frames = 0;
                    context_->pix_fmt = oil_to_avformat( result->pf( ) );
                    context_->flags |= CODEC_FLAG_INTERLACED_ME | CODEC_FLAG_INTERLACED_DCT;
                    context_->flags2 |= CODEC_FLAG2_INTRA_VLC;
                    context_->intra_dc_precision = 1;
                    context_->thread_count = 4;
                }
                else if ( is_dv( pl::to_string( prop_codec_.value< pl::wstring >( ) ) ) )
                {
                    std::string codec =  pl::to_string( prop_codec_.value< pl::wstring >( ) );
                    instance_ = avcodec_find_encoder( CODEC_ID_DVVIDEO );
                    context_->pix_fmt = oil_to_avformat( result->pf( ) );
                    if ( codec == "dv25" )
                        context_->bit_rate = 25000000;
                    else if ( codec == "dv50" )
                        context_->bit_rate = 50000000;
                    else if ( codec == "dvcprohd_1080i" )
                        context_->bit_rate = 25000000;
                }
                else
                {
                    std::cerr << "Failed to create a codec of " << pl::to_string( prop_codec_.value< pl::wstring >( ) ) << std::endl;
                    return;
                }

                context_->width = result->width( );
                context_->height = result->height( );
                AVRational avr = { result->get_fps_den( ), result->get_fps_num( ) };
                context_->time_base = avr;
                AVRational sar = { result->get_sar_num( ), result->get_sar_den( ) };
                context_->sample_aspect_ratio = sar;

                if ( avcodec_open( context_, instance_ ) < 0 ) 
                {
                    std::cerr << "Unable to open codec" << std::endl;
                    return;
                }

                if ( context_->thread_count )
                    avcodec_thread_init( context_, context_->thread_count );

                outbuf_size_ = 5000000;
                outbuf_ = ( boost::uint8_t * )malloc( outbuf_size_ );
            }

            picture_->data[ 0 ] = result->get_image( )->data( 0 );
            picture_->data[ 1 ] = result->get_image( )->data( 1 );
            picture_->data[ 2 ] = result->get_image( )->data( 2 );
            picture_->linesize[ 0 ] = result->get_image( )->pitch( 0 );
            picture_->linesize[ 1 ] = result->get_image( )->pitch( 1 );
            picture_->linesize[ 2 ] = result->get_image( )->pitch( 2 );

            if ( result->get_image( )->field_order( ) != il::progressive )
            {
                picture_->interlaced_frame  = 1;
                picture_->top_field_first = result->get_image( )->field_order( ) != il::top_field_first;
            }

            int out_size = avcodec_encode_video( context_, outbuf_, outbuf_size_, picture_ );

            if ( context_->coded_frame && context_->coded_frame->key_frame )
                key_ = get_position( );

            ml::stream_type_ptr packet = ml::stream_type_ptr( new stream_avformat( pl::to_string( prop_codec_.value< pl::wstring >( ) ), 
                                                                                   out_size, 
                                                                                   get_position( ), 
                                                                                   key_, 
                                                                                   context_->bit_rate, 
                                                                                   ml::dimensions( result->width( ), result->height( ) ), 
                                                                                   ml::fraction( result->get_sar_num( ), result->get_sar_den( ) ), 
                                                                                   result->get_image()->pf() ) );
            memcpy( packet->bytes( ), outbuf_, out_size );
            result->set_stream( packet );
        }

    private:
        pl::pcos::property prop_enable_;
        pl::pcos::property prop_force_;
        pl::pcos::property prop_codec_;
        bool initialised_;
        ml::frame_type_ptr last_frame_;
        ml::input_type_ptr pusher_;
        ml::filter_type_ptr render_;
        bool encoding_;
        AVCodecContext *context_;
        AVCodec *instance_;
        AVFrame *picture_;
        int outbuf_size_;
        boost::uint8_t *outbuf_;
        int key_;
        pl::wstring pf_;
};

filter_type_ptr ML_PLUGIN_DECLSPEC create_avdecode( const pl::wstring & )
{
    return filter_type_ptr( new avformat_decode_filter( ) );
}

filter_type_ptr ML_PLUGIN_DECLSPEC create_avencode( const pl::wstring & )
{
    return filter_type_ptr( new avformat_encode_filter( ) );
}

} } }

