// AMF Stack Input
//
// Copyright (C) 2007 Ardendo
// Released under the terms of the LGPL.
//
// #input:aml_stack:
//
// Provides a simplistic Reverse Polish Notation parser for filter graph 
// generation. 
//
// The concept behind this input is to provide a general testing mechanism 
// which will allow the generation and build of any static graph. 
//
// There are two modes of use. The RPN can be authored in a text file with a
// .aml extension or the graph can be constructed interactively via the 
// exposed 'command' and 'result' properties.
//
// The grammar rules for the language are:
//
// <input> [ <input> | <filter> | <property> | . ] *
//
// Where:
//
// <input> is a rule which creates any aml input object (eg. a file name or 
// url). When included in a .aml file, any file name which includes spaces 
// should be enclosed in double quotes.
//
// <filter> takes the format filter:<name> where name is a rule which creates
// any aml filter object.
//
// <property> takes the form of <name>=<value> where:
//
// <name> is an existing property in the input or filter at the top of the 
// stack and value is the string representation of its value - this is the
// same as assigning a constant value to a property.
//
// or <property> takes the form @<filter-property>=@@<frame-property>[:value]
// where:
//
// <filter-property> is an existing property in the filter at the top of the 
// stack and the <frame-property> is a named property on the frame (normally
// introduced via the keyframe filter). This is the same as associating 
// the property with a variable.
//
// In either case, the right hand side can be enclosed by double quotes should
// spaces be required anywhere.
//
// . is used to signify that the graph on the stack should be evaluated and 
// used for subsequent calls to the input object which holds the stack.
//
// Additionally, the command stucture has been supplemented with a collection
// of FORTH syntax which allows word/function creation, recursion, loops and
// conditional logic, as well as a complement of artithmetic and stack 
// manipulation functions.

#include "precompiled_headers.hpp"
#ifdef WIN32
#pragma warning(push)
// 'this' : used in base member initializer list
#pragma warning(disable:4355)
#endif

#include <boost/filesystem.hpp>

#include "amf_filter_plugin.hpp"
#include "utility.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <deque>
#include <map>
#include <cmath>

#include <openpluginlib/pl/openpluginlib.hpp>
#include <openpluginlib/pl/utf8_utils.hpp>
#include <openimagelib/il/openimagelib_plugin.hpp>
#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/packet.hpp>

#include <opencorelib/cl/str_util.hpp>

#include <boost/regex.hpp>

namespace ml = olib::openmedialib::ml;
namespace il = olib::openimagelib::il;
namespace pl = olib::openpluginlib;
namespace cl = olib::opencorelib;
namespace fs = boost::filesystem;

#ifdef AMF_ON_WINDOWS
#define stack_popen _popen
#define stack_pclose _pclose
#else
#define stack_popen popen
#define stack_pclose pclose
#endif

namespace aml { namespace openmedialib {

bool contains_sub_thread( ml::input_type_ptr input )
{
	bool result = false;
	if ( input )
	{
		result = input->get_uri( ) == L"threader";
		for ( size_t i = 0; !result && i < input->slot_count( ); i ++ )
		{
			ml::input_type_ptr inner = input->fetch_slot( i );
			if ( inner )
			{
				result = inner->get_uri( ) == L"threader";
				if ( !result && inner->property( "threaded" ).valid( ) )
					result = inner->property( "threaded" ).value< int >( ) == 1;
				if ( !result )
					result = contains_sub_thread( inner );
			}
		}
	}
	return result;
}

void activate_sub_thread( ml::input_type_ptr input, int value )
{
	bool result = false;
	if ( input )
	{
		result = input->get_uri( ) == L"threader";
		if ( result )
			input->property( "active" ) = value;
		for ( size_t i = 0; !result && i < input->slot_count( ); i ++ )
		{
			ml::input_type_ptr inner = input->fetch_slot( i );
			if ( inner )
			{
				result = inner->get_uri( ) == L"threader";
				if ( result )
					inner->property( "active" ) = value;
				else
					activate_sub_thread( inner, value );
			}
		}
	}
}

static pl::pcos::key key_deferred_ = pl::pcos::key::from_string( "deferred" );
static pl::pcos::key key_handled_ = pl::pcos::key::from_string( "handled" );
static pl::pcos::key key_query_ = pl::pcos::key::from_string( "query" );
static pl::pcos::key key_token_ = pl::pcos::key::from_string( "token" );
static pl::pcos::key key_stdout_ = pl::pcos::key::from_string( "stdout" );
static pl::pcos::key key_redirect_ = pl::pcos::key::from_string( "redirect" );
static pl::pcos::key key_aml_value_ = pl::pcos::key::from_string( ".aml_value" );
static pl::pcos::key key_slots_ = pl::pcos::key::from_string( "slots" );

static boost::regex int_syntax( "^((?:)|-|\\+)[[:digit:]]+((?:))$" );
static boost::regex double_syntax( "^((?:)|-|\\+)[[:digit:]]+((?:)|\\.[[:digit:]]+)$" );
static boost::regex numeric_syntax( "^((?:)|-|\\+)[[:digit:]]+((?:)|\\.[[:digit:]]+)$" );
static olib::t_regex aml_syntax( _CT("^(aml:|http:).*") );
static olib::t_regex prop_syntax( _CT("^.*=.*") );

class input_value : public ml::input_type
{
	public:
		input_value( const pl::wstring &value )
			: input_type( )
			, value_( value )
		{
			properties( ).remove( properties( ).get_property_with_string( "debug" ) );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		// Basic information
		virtual const pl::wstring get_uri( ) const { return value_; }
		virtual const pl::wstring get_mime_type( ) const { return L"text/value"; }

		// Audio/Visual
		virtual int get_frames( ) const { return 0; }
		virtual bool is_seekable( ) const { return true; }
		virtual int get_video_streams( ) const { return 0; }
		virtual int get_audio_streams( ) const { return 0; }

	protected:
		void do_fetch( ml::frame_type_ptr &f ) { f = ml::frame_type_ptr( new ml::frame_type( ) ); }

	private:
		pl::wstring value_;
};

static pl::pcos::key key_input_video_streams_ = pl::pcos::key::from_string( "@input_video_streams" );
static pl::pcos::key key_input_audio_streams_ = pl::pcos::key::from_string( "@input_audio_streams" );
static pl::pcos::key key_input_slots_ = pl::pcos::key::from_string( "@input_slots" );
static pl::pcos::key key_input_position_ = pl::pcos::key::from_string( "@input_position" );
static pl::pcos::key key_input_frames_ = pl::pcos::key::from_string( "@input_frames" );

static pl::pcos::key key_frame_position_ = pl::pcos::key::from_string( "@frame_position" );
static pl::pcos::key key_frame_fps_num_ = pl::pcos::key::from_string( "@frame_fps_num" );
static pl::pcos::key key_frame_fps_den_ = pl::pcos::key::from_string( "@frame_fps_den" );
static pl::pcos::key key_frame_has_stream_ = pl::pcos::key::from_string( "@frame_has_stream" );
static pl::pcos::key key_frame_has_image_ = pl::pcos::key::from_string( "@frame_has_image" );
static pl::pcos::key key_frame_has_audio_ = pl::pcos::key::from_string( "@frame_has_audio" );
static pl::pcos::key key_frame_is_deferred_ = pl::pcos::key::from_string( "@frame_is_deferred" );
static pl::pcos::key key_frame_pf_ = pl::pcos::key::from_string( "@frame_pf" );
static pl::pcos::key key_frame_width_ = pl::pcos::key::from_string( "@frame_width" );
static pl::pcos::key key_frame_height_ = pl::pcos::key::from_string( "@frame_height" );

static pl::pcos::key key_stream_id_ = pl::pcos::key::from_string( "@stream_id" );
static pl::pcos::key key_stream_container_ = pl::pcos::key::from_string( "@stream_container" );
static pl::pcos::key key_stream_codec_ = pl::pcos::key::from_string( "@stream_codec" );
static pl::pcos::key key_stream_length_ = pl::pcos::key::from_string( "@stream_length" );
static pl::pcos::key key_stream_key_ = pl::pcos::key::from_string( "@stream_key" );
static pl::pcos::key key_stream_position_ = pl::pcos::key::from_string( "@stream_position" );
static pl::pcos::key key_stream_bitrate_ = pl::pcos::key::from_string( "@stream_bitrate" );
static pl::pcos::key key_stream_width_ = pl::pcos::key::from_string( "@stream_width" );
static pl::pcos::key key_stream_height_ = pl::pcos::key::from_string( "@stream_height" );
static pl::pcos::key key_stream_sar_num_ = pl::pcos::key::from_string( "@stream_sar_num" );
static pl::pcos::key key_stream_sar_den_ = pl::pcos::key::from_string( "@stream_sar_den" );
static pl::pcos::key key_stream_pf_ = pl::pcos::key::from_string( "@stream_pf" );
static pl::pcos::key key_stream_frequency_ = pl::pcos::key::from_string( "@stream_frequency" );
static pl::pcos::key key_stream_channels_ = pl::pcos::key::from_string( "@stream_channels" );
static pl::pcos::key key_stream_samples_ = pl::pcos::key::from_string( "@stream_samples" );

static pl::pcos::key key_image_pf_ = pl::pcos::key::from_string( "@image_pf" );
static pl::pcos::key key_image_width_ = pl::pcos::key::from_string( "@image_width" );
static pl::pcos::key key_image_height_ = pl::pcos::key::from_string( "@image_height" );
static pl::pcos::key key_image_pitch_ = pl::pcos::key::from_string( "@image_pitch" );
static pl::pcos::key key_image_linesize_ = pl::pcos::key::from_string( "@image_linesize" );
static pl::pcos::key key_image_position_ = pl::pcos::key::from_string( "@image_position" );
static pl::pcos::key key_image_field_order_ = pl::pcos::key::from_string( "@image_field_order" );
static pl::pcos::key key_image_planes_ = pl::pcos::key::from_string( "@image_planes" );
static pl::pcos::key key_image_size_ = pl::pcos::key::from_string( "@image_size" );
static pl::pcos::key key_image_sar_num_ = pl::pcos::key::from_string( "@image_sar_num" );
static pl::pcos::key key_image_sar_den_ = pl::pcos::key::from_string( "@image_sar_den" );

static pl::pcos::key key_audio_af_ = pl::pcos::key::from_string( "@audio_af" );
static pl::pcos::key key_audio_position_ = pl::pcos::key::from_string( "@audio_position" );
static pl::pcos::key key_audio_frequency_ = pl::pcos::key::from_string( "@audio_frequency" );
static pl::pcos::key key_audio_channels_ = pl::pcos::key::from_string( "@audio_channels" );
static pl::pcos::key key_audio_samples_ = pl::pcos::key::from_string( "@audio_samples" );
static pl::pcos::key key_audio_size_ = pl::pcos::key::from_string( "@audio_size" );

class frame_value : public ml::input_type
{
	public:
		frame_value( const ml::frame_type_ptr &value, ml::input_type_ptr &input )
			: input_type( )
			, prop_input_video_streams_( key_input_video_streams_ )
			, prop_input_audio_streams_( key_input_audio_streams_ )
			, prop_input_slots_( key_input_slots_ )
			, prop_input_frames_( key_input_frames_ )
			, prop_input_position_( key_input_position_ )
			, prop_frame_position_( key_frame_position_ )
			, prop_frame_fps_num_( key_frame_fps_num_ )
			, prop_frame_fps_den_( key_frame_fps_den_ )
			, prop_frame_has_stream_( key_frame_has_stream_ )
			, prop_frame_has_image_( key_frame_has_image_ )
			, prop_frame_has_audio_( key_frame_has_audio_ )
			, prop_frame_is_deferred_( key_frame_is_deferred_ )
			, prop_stream_id_( key_stream_id_ )
			, prop_stream_container_( key_stream_container_ )
			, prop_stream_codec_( key_stream_codec_ )
			, prop_stream_length_( key_stream_length_ )
			, prop_stream_key_( key_stream_key_ )
			, prop_stream_position_( key_stream_position_ )
			, prop_stream_bitrate_( key_stream_bitrate_ )
			, prop_stream_width_( key_stream_width_ )
			, prop_stream_height_( key_stream_height_ )
			, prop_stream_sar_num_( key_stream_sar_num_ )
			, prop_stream_sar_den_( key_stream_sar_den_ )
			, prop_stream_pf_( key_stream_pf_ )
			, prop_stream_frequency_( key_stream_frequency_ )
			, prop_stream_channels_( key_stream_channels_ )
			, prop_stream_samples_( key_stream_samples_ )
			, prop_image_pf_( key_image_pf_ )
			, prop_image_width_( key_image_width_ )
			, prop_image_height_( key_image_height_ )
			, prop_image_pitch_( key_image_pitch_ )
			, prop_image_linesize_( key_image_linesize_ )
			, prop_image_position_( key_image_position_ )
			, prop_image_field_order_( key_image_field_order_ )
			, prop_image_planes_( key_image_planes_ )
			, prop_image_size_( key_image_size_ )
			, prop_image_sar_num_( key_image_sar_num_ )
			, prop_image_sar_den_( key_image_sar_den_ )
			, prop_audio_af_( key_audio_af_ )
			, prop_audio_position_( key_audio_position_ )
			, prop_audio_frequency_( key_audio_frequency_ )
			, prop_audio_channels_( key_audio_channels_ )
			, prop_audio_samples_( key_audio_samples_ )
			, prop_audio_size_( key_audio_size_ )
		{
			properties( ).remove( properties( ).get_property_with_string( "debug" ) );

			if ( input )
			{
				properties( ).append( prop_input_video_streams_ = input->get_video_streams( ) );
				properties( ).append( prop_input_audio_streams_ = input->get_audio_streams( ) );
				properties( ).append( prop_input_slots_ = int( input->slot_count( ) ) );
				properties( ).append( prop_input_position_ = int( input->get_position( ) ) );
				properties( ).append( prop_input_frames_ = int( input->get_frames( ) ) );
			}

			if ( value )
			{
				value_ = value->shallow( );

				properties( ).append( prop_frame_position_ = value_->get_position( ) );
				properties( ).append( prop_frame_fps_num_ = value_->get_fps_num( ) );
				properties( ).append( prop_frame_fps_den_ = value_->get_fps_den( ) );
				properties( ).append( prop_frame_has_stream_ = int( value_->get_stream( ) != ml::stream_type_ptr( ) ) );
				properties( ).append( prop_frame_has_image_ = int( value_->has_image( ) ) );
				properties( ).append( prop_frame_has_audio_ = int( value_->has_audio( ) ) );
				properties( ).append( prop_frame_is_deferred_ = int( value_->is_deferred( ) ) );

				if ( value_->get_stream( ) )
				{
					ml::stream_type_ptr stream = value_->get_stream( );
					properties( ).append( prop_stream_id_ = int( stream->id( ) ) );
					properties( ).append( prop_stream_container_ = pl::to_wstring( stream->container( ) ) );
					properties( ).append( prop_stream_codec_ = pl::to_wstring( stream->codec( ) ) );
					properties( ).append( prop_stream_length_ = int( stream->length( ) ) );
					properties( ).append( prop_stream_key_ = stream->key( ) );
					properties( ).append( prop_stream_position_ = stream->position( ) );
					properties( ).append( prop_stream_bitrate_ = stream->bitrate( ) );
					properties( ).append( prop_stream_width_ = stream->size( ).width );
					properties( ).append( prop_stream_height_ = stream->size( ).height );
					properties( ).append( prop_stream_sar_num_ = stream->sar( ).num );
					properties( ).append( prop_stream_sar_den_ = stream->sar( ).den );
					properties( ).append( prop_stream_pf_ = stream->pf( ) );
					properties( ).append( prop_stream_frequency_ = stream->frequency( ) );
					properties( ).append( prop_stream_channels_ = stream->channels( ) );
					properties( ).append( prop_stream_samples_ = stream->samples( ) );
				}

				if ( value_->has_image( ) )
				{
					il::image_type_ptr image = value_->get_image( );
					properties( ).append( prop_image_pf_ = image->pf( ) );
					properties( ).append( prop_image_width_ = image->width( ) );
					properties( ).append( prop_image_height_ = image->height( ) );
					properties( ).append( prop_image_pitch_ = image->pitch( ) );
					properties( ).append( prop_image_linesize_ = image->linesize( ) );
					properties( ).append( prop_image_position_ = image->position( ) );
					properties( ).append( prop_image_field_order_ = int( image->field_order( ) ) );
					properties( ).append( prop_image_planes_ = int( image->plane_count( ) ) );
					properties( ).append( prop_image_size_ = image->size( ) );
					properties( ).append( prop_image_sar_num_ = value->get_sar_num( ) );
					properties( ).append( prop_image_sar_den_ = value->get_sar_den( ) );
				}

				if ( value_->has_audio( ) )
				{
					ml::audio_type_ptr audio = value_->get_audio( );
					properties( ).append( prop_audio_af_ = audio->af( ) );
					properties( ).append( prop_audio_position_ = audio->position( ) );
					properties( ).append( prop_audio_frequency_ = audio->frequency( ) );
					properties( ).append( prop_audio_channels_ = audio->channels( ) );
					properties( ).append( prop_audio_samples_ = audio->samples( ) );
					properties( ).append( prop_audio_size_ = audio->size( ) );
				}
			}
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		// Basic information
		virtual const pl::wstring get_uri( ) const { return L"frame:"; }
		virtual const pl::wstring get_mime_type( ) const { return L"frame/value"; }

		// Audio/Visual
		virtual int get_frames( ) const { return value_ ? 1 : 0; }
		virtual bool is_seekable( ) const { return true; }
		virtual int get_video_streams( ) const { return value_ && value_->has_image( ) ? 1 : 0; }
		virtual int get_audio_streams( ) const { return value_ && value_->has_audio( ) ? 1 : 0; }

	protected:
		void do_fetch( ml::frame_type_ptr &f ) { if ( value_ && get_position( ) == 0 ) { f = value_->shallow( ); f->set_position( 0 ); } }

	private:
		ml::frame_type_ptr value_;
		pl::pcos::property prop_input_video_streams_;
		pl::pcos::property prop_input_audio_streams_;
		pl::pcos::property prop_input_slots_;
		pl::pcos::property prop_input_frames_;
		pl::pcos::property prop_input_position_;
		pl::pcos::property prop_frame_position_;
		pl::pcos::property prop_frame_fps_num_;
		pl::pcos::property prop_frame_fps_den_;
		pl::pcos::property prop_frame_has_stream_;
		pl::pcos::property prop_frame_has_image_;
		pl::pcos::property prop_frame_has_audio_;
		pl::pcos::property prop_frame_is_deferred_;
		pl::pcos::property prop_stream_id_;
		pl::pcos::property prop_stream_container_;
		pl::pcos::property prop_stream_codec_;
		pl::pcos::property prop_stream_length_;
		pl::pcos::property prop_stream_key_;
		pl::pcos::property prop_stream_position_;
		pl::pcos::property prop_stream_bitrate_;
		pl::pcos::property prop_stream_width_;
		pl::pcos::property prop_stream_height_;
		pl::pcos::property prop_stream_sar_num_;
		pl::pcos::property prop_stream_sar_den_;
		pl::pcos::property prop_stream_pf_;
		pl::pcos::property prop_stream_frequency_;
		pl::pcos::property prop_stream_channels_;
		pl::pcos::property prop_stream_samples_;
		pl::pcos::property prop_image_pf_;
		pl::pcos::property prop_image_width_;
		pl::pcos::property prop_image_height_;
		pl::pcos::property prop_image_pitch_;
		pl::pcos::property prop_image_linesize_;
		pl::pcos::property prop_image_position_;
		pl::pcos::property prop_image_field_order_;
		pl::pcos::property prop_image_planes_;
		pl::pcos::property prop_image_size_;
		pl::pcos::property prop_image_sar_num_;
		pl::pcos::property prop_image_sar_den_;
		pl::pcos::property prop_audio_af_;
		pl::pcos::property prop_audio_position_;
		pl::pcos::property prop_audio_frequency_;
		pl::pcos::property prop_audio_channels_;
		pl::pcos::property prop_audio_samples_;
		pl::pcos::property prop_audio_size_;
};

template < typename T > class stack_value : public ml::input_type
{
	public:
		stack_value( const T &value )
			: input_type( )
			, prop_value_( key_aml_value_ )
			, converted_( false )
		{
			properties( ).remove( properties( ).get_property_with_string( "debug" ) );
			properties( ).append( prop_value_ = value );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		// Basic information
		virtual const pl::wstring get_uri( ) const 
		{ 
			// Delay converting until requested
			if ( !converted_ )
			{
				std::ostringstream stream;
				stream.precision( 24 );
				stream << prop_value_.value< T >( );
				string_ = pl::to_wstring( stream.str( ) );
				converted_ = true;
			}
			return string_; 
		}
		virtual const pl::wstring get_mime_type( ) const { return L"text/value"; }

		// Audio/Visual
		virtual int get_frames( ) const { return 0; }
		virtual bool is_seekable( ) const { return true; }
		virtual int get_video_streams( ) const { return 0; }
		virtual int get_audio_streams( ) const { return 0; }

	protected:
		void do_fetch( ml::frame_type_ptr &f ) { f = ml::frame_type_ptr( new ml::frame_type( ) ); }

	private:
		pl::pcos::property prop_value_;
		mutable pl::wstring string_;
		mutable bool converted_;
};

struct sequence
{
	std::vector< pl::wstring > words_;
	std::vector< pl::wstring >::iterator iter_;
	bool parseable_;

	sequence( bool parseable = false )
		: words_( )
		, iter_( words_.begin( ) )
		, parseable_( parseable )
	{ }

	sequence( const std::vector< pl::wstring > &words, bool parseable = false )
		: words_( words )
		, iter_( words_.begin( ) )
		, parseable_( parseable )
	{ }

	sequence( const pl::wstring_list &words, bool parseable = false )
		: parseable_( parseable )
	{ 
		for ( pl::wstring_list::const_iterator i = words.begin( ); i != words.end( ); i ++ )
			words_.push_back( *i );
		iter_ = words_.begin( );
	}

	sequence( const sequence &other, bool parseable = false )
		: words_( other.words_ )
		, iter_( words_.begin( ) )
		, parseable_( parseable )
	{ }

	void parseable( bool value )
	{ parseable_ = value; }

	bool is_parseable( ) const
	{ return parseable_; }

	void add( const pl::wstring &word )
	{ words_.push_back( word ); }

	void start( )
	{ iter_ = words_.begin( ); }

	void end( )
	{ iter_ = words_.end( ); }

	bool done( ) const
	{ return iter_ == words_.end( ); }

	const pl::wstring &next( )
	{ return *( iter_ ++ ); }
};

typedef boost::shared_ptr< sequence > sequence_ptr;

static sequence_ptr dummy_sequence_ = sequence_ptr( new sequence( ) );

class aml_stack;

static void mc_workaround( aml_stack * );

// Word definition operators
static void op_dot( aml_stack * );
static void op_colon( aml_stack * );
static void op_semi_colon( aml_stack * );
static void op_forget( aml_stack * );
static void op_str( aml_stack * );
static void op_execute( aml_stack * );

static void op_throw( aml_stack * );
static void op_parse( aml_stack * );
static void op_parseable( aml_stack * );
static void op_handler( aml_stack * );

// Arithmetic operators
static void op_plus( aml_stack * );
static void op_minus( aml_stack * );
static void op_div( aml_stack * );
static void op_mult( aml_stack * );
static void op_pow( aml_stack * );
static void op_mod( aml_stack * );
static void op_abs( aml_stack * );
static void op_floor( aml_stack * );
static void op_ceil( aml_stack * );
static void op_sin( aml_stack * );
static void op_cos( aml_stack * );
static void op_tan( aml_stack * );

// Comparison operators
static void op_equal( aml_stack * );
static void op_is( aml_stack * );
static void op_lt( aml_stack * );
static void op_gt( aml_stack * );
static void op_lt_equal( aml_stack * );
static void op_gt_equal( aml_stack * );

// Stack operators
static void op_eval( aml_stack * );
static void op_drop( aml_stack * );
static void op_pick( aml_stack * );
static void op_roll( aml_stack * );
static void op_shift( aml_stack * );
static void op_depth( aml_stack * );

// Return stack operators
static void op_rpush( aml_stack * );
static void op_rpop( aml_stack * );
static void op_rdup( aml_stack * );
static void op_rdrop( aml_stack * );
static void op_rdepth( aml_stack * );
static void op_rpick( aml_stack * );
static void op_rroll( aml_stack * );

// Flow control operators
static void op_if( aml_stack * );
static void op_else( aml_stack * );
static void op_then( aml_stack * );

// Loop operators
static void op_begin( aml_stack * );
static void op_until( aml_stack * );
static void op_while( aml_stack * );
static void op_repeat( aml_stack * );

static void op_pack( aml_stack * );
static void op_pack_append( aml_stack * );
static void op_pack_append_list( aml_stack * );
static void op_pack_insert( aml_stack * );
static void op_pack_remove( aml_stack * );
static void op_pack_query( aml_stack * );
static void op_pack_assign( aml_stack * );

static void op_iter_start( aml_stack * );
static void op_iter_slots( aml_stack * );
static void op_iter_frames( aml_stack * );
static void op_iter_range( aml_stack * );
static void op_iter_popen( aml_stack * );
static void op_iter_props( aml_stack * );
static void op_iter_aml( aml_stack * );

// Variables
static void op_variable( aml_stack * );
static void op_variable_assign( aml_stack * );
static void op_assign( aml_stack * );
static void op_deref( aml_stack * );

// ml operators
static void op_prop_query( aml_stack * );
static void op_prop_exists( aml_stack * );
static void op_prop_matches( aml_stack * );
static void op_prop_query_parse( aml_stack * );
static void op_has_prop_parse( aml_stack * );
static void op_length( aml_stack * );
static void op_decap( aml_stack * );
static void op_slots( aml_stack * );
static void op_connect( aml_stack * );
static void op_slot( aml_stack * );
static void op_recover( aml_stack * );
static void op_fetch( aml_stack * );

// Debug operators
static void op_available( aml_stack * );
static void op_dump( aml_stack * );
static void op_rdump( aml_stack * );
static void op_tos( aml_stack * );
static void op_trace( aml_stack * );
static void op_words( aml_stack * );
static void op_dict( aml_stack * );
static void op_define( aml_stack * );
static void op_log_level( aml_stack * );
static void op_render( aml_stack * );
static void op_donor( aml_stack * );
static void op_transplant( aml_stack * );

// OS operatots
static void op_path( aml_stack * );
static void op_popen( aml_stack * );

typedef void ( *operation )( aml_stack * );

struct aml_operation
{
	aml_operation( )
		: operation_( 0 )
		, description_( "" )
	{ }

	aml_operation( operation oper, const std::string &description = "" )
		: operation_( oper )
		, description_( description )
	{ }

	void run( aml_stack *stack )
	{ operation_( stack ); }

	const std::string &description( ) const
	{ return description_; }

	operation operation_;
	std::string description_;
};

class aml_stack 
{
	public:

		aml_stack( ml::input_type *parent, pl::pcos::property &prop_stdout )
			: parent_( parent )
			, prop_stdout_( prop_stdout )
			, output_( 0 )
			, inputs_( )
			, words_( )
			, word_( )
			, variables_( )
			, word_count_( 0 )
			, state_( 0 )
			, loop_count_( 0 )
			, trace_( 0 )
			, next_op_( )
			, next_exec_op_( 0 )
			, ignore_( 0 )
		{
			paths_.push_back( fs::initial_path< olib::t_path >( ) );

			operations_[ L"mc_workaround" ] = aml_operation( mc_workaround, "Initialise mainconcept workaround" );

			operations_[ L"." ] = aml_operation( op_dot, "Use the input at the top of the stack (<input> . ==)" );
			operations_[ L"dot" ] = aml_operation( op_dot, "Use the input at the top of the stack (<input> dot ==)" );

			operations_[ L":" ] = aml_operation( op_colon, "Start a word definition" );
			operations_[ L";" ] = aml_operation( op_semi_colon, "End a word definition" );

			operations_[ L"forget" ] = aml_operation( op_forget, "Removes the following word definition" );
			operations_[ L"$" ] = aml_operation( op_str, "Places the following string on the stack" );
			operations_[ L"str" ] = aml_operation( op_str, "Places the following string on the stack" );
			operations_[ L"execute" ] = aml_operation( op_execute, "Execute the packed strings (<packed-array> execute ==)" );
			operations_[ L"throw" ] = aml_operation( op_throw, "Throws an exception containing the following string" );
			operations_[ L"parse" ] = aml_operation( op_parse, "Puts the next string from the input stream on the stack" );
			operations_[ L"parseable" ] = aml_operation( op_parseable, "Indicates that a word definition provides an input stream to called words which invoke parse" );

			operations_[ L"+" ] = aml_operation( op_plus, "Adds two numbers (<a> <b> + == <a+b>)" );
			operations_[ L"-" ] = aml_operation( op_minus, "Subtracts two numbers (<a> <b> - == <a+b>)" );
			operations_[ L"/" ] = aml_operation( op_div, "Divides two numbers (<a> <b> * == <a+b>)" );
			operations_[ L"*" ] = aml_operation( op_mult, "Multiplies two numbers (<a> <b> / == <a+b>)" );
			operations_[ L"**" ] = aml_operation( op_pow, "Raises to the power of (<a> <b> ** == <a**b>" );
			operations_[ L"mod" ] = aml_operation( op_mod, "Modulo of two numbers (<a> <b> mod == <a mod b>" );
			operations_[ L"abs" ] = aml_operation( op_abs, "Absolute value (<a> abs == <abs a>" );
			operations_[ L"floor" ] = aml_operation( op_floor, "Largest integral value not greater than TOS (<a> floor == <floor a>)"  );
			operations_[ L"ceil" ] = aml_operation( op_ceil, "Smallest integral value not less than TOS (<a> ceil == <ceil a>)" );
			operations_[ L"sin" ] = aml_operation( op_sin, "Sine of TOS (<a> sin == <sin a>" );
			operations_[ L"cos" ] = aml_operation( op_cos, "Cosine of TOS (<a> cos == <cos a>" );
			operations_[ L"tan" ] = aml_operation( op_tan, "Tangent of TOS (<a> tan == <tan a>" );

			operations_[ L"eval" ] = aml_operation( op_eval, "Forces a pop/push evaluation (<a> eval == <a>" );
			operations_[ L"drop" ] = aml_operation( op_drop, "Drops the item at the top of the stack (<a> drop ==)" );
			operations_[ L"pick" ] = aml_operation( op_pick, "Picks the Nth item from the stack (<N> .. <n> pick == <N> ... <N>)" );
			operations_[ L"roll" ] = aml_operation( op_roll, "Moves the Nth item to the top of the stack (<N> .. <n> roll == ... <N>)" );
			operations_[ L"shift" ] = aml_operation( op_shift, "Moves the TOS to the Nth item (<N> ... <a> <n> shift == <a> <N> ...)"  );
			operations_[ L"depth?" ] = aml_operation( op_depth, "Places the number of items on the stack at the TOS (... depth? == ... <n>)" );

			operations_[ L">r" ] = aml_operation( op_rpush, "Moves TOS to the return stack" );
			operations_[ L"r>" ] = aml_operation( op_rpop, "Moves TOS of the return stack to stack" );
			operations_[ L"r@" ] = aml_operation( op_rdup, "Copies TOS of the return stack to the stack" );
			operations_[ L"rdrop" ] = aml_operation( op_rdrop, "Drops the TOS of the return stack" );
			operations_[ L"rdepth?" ] = aml_operation( op_rdepth, "Puts the number of items on the return stack on top of the stack" );
			operations_[ L"rpick" ] = aml_operation( op_rpick );
			operations_[ L"rroll" ] = aml_operation( op_rroll );

			operations_[ L"if" ] = aml_operation( op_if, "Start of a logic branch (<a> if ... ==)" );
			operations_[ L"else" ] = aml_operation( op_else, "Optional else clause for a logic branch" );
			operations_[ L"then" ] = aml_operation( op_then, "Closure of a logic branch" );
			operations_[ L"=" ] = aml_operation( op_equal, "Equality check (<a> <b> = == <result>)" );
			operations_[ L"is" ] = aml_operation( op_is );
			operations_[ L"<" ] = aml_operation( op_lt );
			operations_[ L">" ] = aml_operation( op_gt );
			operations_[ L"<=" ] = aml_operation( op_lt_equal );
			operations_[ L">=" ] = aml_operation( op_gt_equal );
			operations_[ L">=" ] = aml_operation( op_gt_equal );
			operations_[ L"begin" ] = aml_operation( op_begin );
			operations_[ L"until" ] = aml_operation( op_until );
			operations_[ L"while" ] = aml_operation( op_while );
			operations_[ L"repeat" ] = aml_operation( op_repeat );

			operations_[ L"pack" ] = aml_operation( op_pack );
			operations_[ L"pack&" ] = aml_operation( op_pack_append );
			operations_[ L"pack&&" ] = aml_operation( op_pack_append_list );
			operations_[ L"pack+" ] = aml_operation( op_pack_insert );
			operations_[ L"pack-" ] = aml_operation( op_pack_remove );
			operations_[ L"pack@" ] = aml_operation( op_pack_query );
			operations_[ L"pack!" ] = aml_operation( op_pack_assign );

			operations_[ L"iter_start" ] = aml_operation( op_iter_start );
			operations_[ L"iter_slots" ] = aml_operation( op_iter_slots );
			operations_[ L"iter_frames" ] = aml_operation( op_iter_frames );
			operations_[ L"iter_range" ] = aml_operation( op_iter_range );
			operations_[ L"iter_popen" ] = aml_operation( op_iter_popen );
			operations_[ L"iter_props" ] = aml_operation( op_iter_props );
			operations_[ L"iter_aml" ] = aml_operation( op_iter_aml );

			operations_[ L"variable" ] = aml_operation( op_variable );
			operations_[ L"variable!" ] = aml_operation( op_variable_assign );
			operations_[ L"!" ] = aml_operation( op_assign );
			operations_[ L"@" ] = aml_operation( op_deref );

			operations_[ L"prop_query" ] = aml_operation( op_prop_query );
			operations_[ L"prop_exists" ] = aml_operation( op_prop_exists );
			operations_[ L"prop_matches" ] = aml_operation( op_prop_matches );
			operations_[ L"?" ] = aml_operation( op_prop_query_parse );
			operations_[ L"has?" ] = aml_operation( op_has_prop_parse );
			operations_[ L"length?" ] = aml_operation( op_length );
			operations_[ L"decap" ] = aml_operation( op_decap );
			operations_[ L"slots?" ] = aml_operation( op_slots );
			operations_[ L"connect" ] = aml_operation( op_connect );
			operations_[ L"slot" ] = aml_operation( op_slot );
			operations_[ L"recover" ] = aml_operation( op_recover );
			operations_[ L"fetch" ] = aml_operation( op_fetch );

			operations_[ L"available" ] = aml_operation( op_available );
			operations_[ L"dump" ] = aml_operation( op_dump );
			operations_[ L"rdump" ] = aml_operation( op_rdump );
			operations_[ L"tos" ] = aml_operation( op_tos );
			operations_[ L"trace" ] = aml_operation( op_trace );
			operations_[ L"words" ] = aml_operation( op_words );
			operations_[ L"dict" ] = aml_operation( op_dict );
			operations_[ L"define" ] = aml_operation( op_define );
			operations_[ L"log_level" ] = aml_operation( op_log_level );
			operations_[ L"render" ] = aml_operation( op_render );
			operations_[ L"donor" ] = aml_operation( op_donor );
			operations_[ L"transplant" ] = aml_operation( op_transplant );

			operations_[ L"popen" ] = aml_operation( op_popen );
			operations_[ L"path" ] = aml_operation( op_path );

			conds_.push_back( true );
			variables_push( );
		}

		~aml_stack( )
		{ }

		void set_output( std::ostringstream *output )
		{
			output_ = output;
		}

		void variables_push( )
		{
			variables_.push_back( std::map< pl::wstring, ml::input_type_ptr >( ) );
		}

		void variables_pop( )
		{
			variables_.pop_back( );
		}

		void assign( pl::pcos::property_container &props, const std::string &name, const pl::wstring &value )
		{
			pl::pcos::key key = pl::pcos::key::from_string( name.c_str( ) );
			pl::pcos::property property = props.get_property_with_key( key );

			if ( property.valid( ) )
			{
				property.set_from_string( value );
				property.update( );
			}
			else if ( name[ 0 ] == '@' )
			{
				pl::pcos::property prop( key );
				props.append( prop = value );
			}
			else
			{
				throw( std::string( "Invalid property " ) + name );
			}
		}

		void assign( pl::pcos::property_container &props, const pl::wstring &pair )
		{
			size_t pos = pair.find( L"=" );
			std::string name = pl::to_string( pair.substr( 0, pos ) );
			pl::wstring value = pair.substr( pos + 1 );
		
			if ( value[ 0 ] == L'"' && value[ value.size( ) - 1 ] == L'"' )
				value = value.substr( 1, value.size( ) - 2 );

			if ( value.find( L"%s" ) != pl::wstring::npos )
			{
				std::deque < ml::input_type_ptr >::iterator iter = inputs_.end( ) - 2;
				value = pl::to_wstring( ( boost::format( pl::to_string( value ) ) % pl::to_string( ( *iter )->get_uri( ) ) ).str( ) );
				inputs_.erase( iter );
			}

			assign( props, name, value );
		}

		void reset( )
		{
			state_ = 0;
			word_count_ = 0;
			loop_count_ = 0;
			word_.erase( word_.begin( ), word_.end( ) );
			loops_.erase( loops_.begin( ), loops_.end( ) );
			next_op_.erase( next_op_.begin( ), next_op_.end( ) );
			execution_stack_.erase( execution_stack_.begin( ), execution_stack_.end( ) );
			loop_cond_.erase( loop_cond_.begin( ), loop_cond_.end( ) );
			conds_.erase( conds_.begin( ), conds_.end( ) );
			conds_.push_back( true );
			while ( variables_.size( ) > 1 )
				variables_pop( );
			next_exec_op_ = 0;
		}

		void push( const pl::wstring &token )
		{
			if ( next_op_.size( ) == 0 && token == L"" ) return;

			if ( int( execution_stack_.size( ) ) + 1 <= trace_ )
			{
				( *output_ ).fill( '#' );
				( *output_ ).width( execution_stack_.size( ) + 2 );
				*output_ << " " << pl::to_string( token ) << std::endl;
			}

			if ( token == L"reset!" )
			{
				reset( );
				throw std::string( "Cancelled and reset state" );
			}

			pl::pcos::property_container properties;

			pl::wstring arg = token;

			// Remove outer " and '
			if ( arg[ 0 ] == L'"' && arg[ arg.size( ) - 1 ] == L'"' )
				arg = arg.substr( 1, arg.size( ) - 2 );
			else if ( arg[ 0 ] == L'\'' && arg[ arg.size( ) - 1 ] == L'\'' )
				arg = arg.substr( 1, arg.size( ) - 2 );

            olib::t_string url = cl::str_util::to_t_string( arg );
			bool is_http_source = paths_.back( ).external_directory_string( ).find( _CT("http://") ) == 0;

            if ( !is_http_source && url != _CT("/") && is_file( paths_.back( ), url ) )
				url = olib::t_path( paths_.back( ) / url ).external_directory_string( );

			if ( inputs_.size( ) )
				properties = inputs_.back( )->properties( );

			if ( ( ( arg != L":" && arg != L";" ) || ignore_ ) && state_ == 1 )
			{
				ignore_ = !ignore_ && arg == L"$" ? ignore_ = 1 : ignore_ = 0;
				if ( arg != L"" )
					word_.push_back( arg );
				else
					word_.push_back( L"\"" + arg + L"\"" );
			}
			else if ( next_op_.size( ) == 0 && inline_.find( arg ) != inline_.end( ) )
				execute_inline( arg );
			else if ( arg != L"begin" && arg != L"until" && arg != L"repeat" && arg.find( L"iter_" ) != 0 && state_ == 2 )
				loops_.push_back( arg );
			else if ( arg != L"if" && arg != L"else" && arg != L"then" && !conds_.back( ) )
				;
			else if ( next_op_.size( ) != 0 )
				execute_operation( arg );
			else if ( get_locals( ).find( arg ) != get_locals( ).end( ) )
				inputs_.push_back( ml::input_type_ptr( new input_value( arg ) ) );
			else if ( handled( arg ) )
				;
			else if ( is_word( arg ) )
				;
			//else if ( words_.find( arg ) != words_.end( ) )
				//execute_word( arg );
			else if ( operations_.find( arg ) != operations_.end( ) )
				execute_operation( arg );
			else if ( boost::regex_match( pl::to_string( arg ), int_syntax ) )
				create_int( arg );
			else if ( boost::regex_match( pl::to_string( arg ), double_syntax ) )
				create_double( arg );
			else if ( boost::regex_match( pl::to_string( arg ), numeric_syntax ) )
				create_numeric( arg );
			else if ( arg.find( L"=" ) != pl::wstring::npos && arg.find( L"http:" ) < arg.find( L"=" ) )
                create_input( cl::str_util::to_wstring( url ) );
			else if ( arg.find( L"=" ) != pl::wstring::npos )
				assign( properties, arg );
			else if ( url.find( _CT("http://") ) == 0 && url.find( _CT(".aml") ) == arg.size( ) - 4 )
				parse_http( url );
			else if ( is_http_source && arg.find( L".aml" ) == arg.size( ) - 4 )
				parse_http( paths_.back( ).external_directory_string( ) + _CT("/") + cl::str_util::to_t_string( arg ) );
			else if ( url.find( _CT(".aml") ) == url.size( ) - 4 || url == _CT( "stdin:" ) )
                parse_file( cl::str_util::to_wstring( url ) );
			else if ( arg.find( L"filter:" ) == 0 )
				create_filter( arg );
			else if ( arg != L"" && arg[ arg.size( ) - 1 ] != L':' && arg.find( L"http://" ) == pl::wstring::npos && is_http_source )
                create_input( cl::str_util::to_wstring( url ) );
			else if ( arg != L"" && is_an_aml_script( arg ) && arg.find( L"http://" ) == pl::wstring::npos )
                parse_file( cl::str_util::to_wstring( url ) );
			else if ( arg != L"" && ( arg[ arg.size( ) - 1 ] == L':' || arg.find( L"http://" ) != pl::wstring::npos ) )
                create_input( arg );
			else if ( arg != L"" )
                create_input( cl::str_util::to_wstring( url ) );
		}

		bool is_word( const pl::wstring &arg )
		{
			bool result = words_.find( arg ) != words_.end( );
			if ( result )
				execute_word( arg );
			return result;
		}

		std::map< pl::wstring, ml::input_type_ptr > &get_locals( )
		{
			return variables_[ variables_.size( ) - 1 ];
		}

		bool is_local( const pl::wstring &name )
		{
			int index = variables_.size( );
			while ( index -- )
			{
				std::map< pl::wstring, ml::input_type_ptr > &current = variables_[ index ];
				if ( current.find( name ) != current.end( ) )
					return true;
			}
			return false;
		}

		void assign_local( const pl::wstring &name, ml::input_type_ptr &value )
		{
			int index = variables_.size( );
			while ( index -- )
			{
				std::map< pl::wstring, ml::input_type_ptr > &current = variables_[ index ];
				if ( current.find( name ) != current.end( ) )
				{
					current[ name ] = value;
					return;
				}
			}
			throw std::string( "Attempt to assign to a non-existentent local var " + pl::to_string( name ) );
		}

		void deref_local( const pl::wstring &name )
		{
			int index = variables_.size( );
			while ( index -- )
			{
				std::map< pl::wstring, ml::input_type_ptr > &current = variables_[ index ];
				if ( current.find( name ) != current.end( ) )
				{
					push( current[ name ] );
					return;
				}
			}
			throw std::string( "Attempt to deref a non-existentent local var " + pl::to_string( name ) );
		}

		bool handled( const pl::wstring &arg )
		{
			execution_stack_.push_back( dummy_sequence_ );
			parent_->properties( ).get_property_with_key( key_handled_ ).set( 0 );
			parent_->properties( ).get_property_with_key( key_query_ ) = arg;
			execution_stack_.pop_back( );
			int status = parent_->properties( ).get_property_with_key( key_handled_ ).value< int >( );
			if ( status == 2 )
				next_op( op_handler );
			return status != 0;
		}

		void push_property( const pl::wstring &arg )
		{
			ml::input_type_ptr input = pop( );
			push( input );
			std::string name = pl::to_string( arg );
			pcos::property prop = input->property( name.c_str( ) );
			if ( prop.valid( ) )
			{
				if ( prop.is_a< double >( ) )
					push( prop.value< double >( ) );
				else if ( prop.is_a< int >( ) )
					push( prop.value< int >( ) );
				else if ( prop.is_a< std::string >( ) )
					inputs_.push_back( ml::input_type_ptr( new input_value( pl::to_wstring( prop.value< std::string >( ) ) ) ) );
				else if ( prop.is_a< pl::wstring >( ) )
					inputs_.push_back( ml::input_type_ptr( new input_value( prop.value< pl::wstring >( ) ) ) );
				else
					throw std::string( "Can't serialise " + name + " to a string" );

				//if ( prop.is_a< bool >( ) ) return str( prop.value_as_bool( ) )
				//if ( prop.is_a< il::image_type_ptr >( ) ) return None
				//if ( prop.is_a_< ml::media_type_ptr >( ) ) return None
				//if ( prop.is_a< boost::int64 >( ) ) return str( prop.value_as_int64( ) )
				//if ( prop.is_a< std::vector< int > >( ) ) return convert_list_to_string( prop.value_as_int_list( ) )
				//if ( prop.is_a< std::vector< std::string > >( ) ) return convert_list_to_string( prop.value_as_string_list( ), True )
				//if ( prop.is_a< boost::uint64_t >( ) ) return str( prop.value_as_uint64( ) )
				//else if ( prop.is_a< std::vector< pl::wstring > >( ) ) return convert_list_to_string( prop.value_as_wstring_list( ), True )
			}
			else
			{
				throw std::string( "Query on a non-existent property " + name );
			}
		}

		void push( ml::input_type_ptr input )
		{
			if ( state_ != 0 )
				throw std::string( "Synatx error - attempt to insert binary input in function" );
			inputs_.push_back( input );
		}

		void push( ml::filter_type_ptr input )
		{
			if ( state_ != 0 )
				throw std::string( "Synatx error - attempt to insert binary input in function" );
			inputs_.push_back( input );
		}

		void push( double value )
		{
			if ( !empty( ) ) push( pop( ) );
			inputs_.push_back( ml::input_type_ptr( new stack_value< double >( value ) ) );
		}

		void push( int value )
		{
			if ( !empty( ) ) push( pop( ) );
			inputs_.push_back( ml::input_type_ptr( new stack_value< int >( value ) ) );
		}

		void push( boost::int64_t value )
		{
			if ( !empty( ) ) push( pop( ) );
			inputs_.push_back( ml::input_type_ptr( new stack_value< boost::int64_t >( value ) ) );
		}

		void push( bool value )
		{
			if ( !empty( ) ) push( pop( ) );
			inputs_.push_back( ml::input_type_ptr( new stack_value< int >( value ? 1 : 0 ) ) );
		}

		bool empty( ) const
		{
			return inputs_.size( ) == 0;
		}

		void set_trace( int level )
		{
			trace_ = level;
		}

		void print_word( const pl::wstring &name )
		{
			if ( inline_.find( name ) != inline_.end( ) )
			{
				std::vector< pl::wstring > function = inline_[ name ];
				*output_ << ": " << pl::to_string( name ) << std::endl << "inline ";
				for ( std::vector< pl::wstring >::iterator i = function.begin( ); i != function.end( ); i ++ )
				{
					if ( ( *i ).find( L" " ) == pl::wstring::npos )
						*output_ << pl::to_string( *i ) << " ";
					else
						*output_ << "\"" << pl::to_string( *i ) << "\" ";
				}
				*output_ << std::endl << ";";
			}
			else if ( words_.find( name ) != words_.end( ) )
			{
				std::vector< pl::wstring > function = words_[ name ];
				*output_ << ": " << pl::to_string( name ) << std::endl;
				for ( std::vector< pl::wstring >::iterator i = function.begin( ); i != function.end( ); i ++ )
				{
					if ( ( *i ).find( L" " ) == pl::wstring::npos )
						*output_ << pl::to_string( *i ) << " ";
					else
						*output_ << "\"" << pl::to_string( *i ) << "\" ";
				}
				*output_ << std::endl << ";";
			}
			*output_ << std::endl;
		}

		void print_words( )
		{
			*output_ << "# Primitives:"  << std::endl << std::endl;
			for ( std::map < pl::wstring, aml_operation >::iterator i = operations_.begin( ); i != operations_.end( ); i ++ )
			{
				if ( words_.find( i->first ) == words_.end( ) )
					*output_ << "# " << pl::to_string( i->first ) << ": " << i->second.description( ) << std::endl;
			}

			*output_ << std::endl << "# Defined Words:" << std::endl << std::endl;
			for( std::map < pl::wstring, std::vector< pl::wstring > >::iterator i = words_.begin( ); i != words_.end( ); i ++ )
			{
				print_word( i->first );
				*output_ << std::endl;
			}
			*output_ << std::endl << "# Inline Words:" << std::endl << std::endl;
			for( std::map < pl::wstring, std::vector< pl::wstring > >::iterator i = inline_.begin( ); i != inline_.end( ); i ++ )
			{
				print_word( i->first );
				*output_ << std::endl;
			}
		}

		void dict( )
		{
			*output_ << "Primitives:"  << std::endl << std::endl;
			for ( std::map < pl::wstring, aml_operation >::iterator i = operations_.begin( ); i != operations_.end( ); i ++ )
			{
				if ( words_.find( i->first ) == words_.end( ) )
				{
					if ( i != operations_.begin( ) )
						*output_ << ", ";
					*output_ << pl::to_string( i->first );
				}
			}

			*output_ << std::endl << std::endl << "Defined Words:" << std::endl << std::endl;
			for( std::map < pl::wstring, std::vector< pl::wstring > >::iterator i = words_.begin( ); i != words_.end( ); i ++ )
			{
				if ( i != words_.begin( ) )
					*output_ << ", ";
				*output_ << pl::to_string( i->first );
			}

			*output_ << std::endl << std::endl << "Inline Words:" << std::endl << std::endl;
			for( std::map < pl::wstring, std::vector< pl::wstring > >::iterator i = inline_.begin( ); i != inline_.end( ); i ++ )
			{
				if ( i != inline_.begin( ) )
					*output_ << ", ";
				*output_ << pl::to_string( i->first );
			}

			*output_ << std::endl;
		}

		void commit_word( )
		{
			if ( state_ == 1 && word_.size( ) >= 1 )
			{
				pl::wstring name = word_[ 0 ];
				word_.erase( word_.begin( ) );

				if ( word_[ 0 ] == L"inline" )
				{
					word_.erase( word_.begin( ) );
					inline_[ name ] = word_;
				}
				else
				{
					words_[ name ] = word_;
				}

				word_.erase( word_.begin( ), word_.end( ) );
				state_ = 0;
			}
			else
			{
				throw std::string( "Syntax error - attempt to commit an empty function" );
			}
		}

		bool next_op( operation op )
		{
			bool result = false;

			if ( next_op_.size( ) != 0 )
				result = next_op_.back( ) == op;

			if ( !result )
				next_op_.push_back( op );
			else
				next_op_.pop_back( );

			return result;
		}

		void forget_word( const pl::wstring &word )
		{
			if ( words_.find( word ) != words_.end( ) )
				words_.erase( words_.find( word ) );
			else if ( inline_.find( word ) != inline_.end( ) )
				inline_.erase( inline_.find( word ) );
			else
				throw std::string( "Word does not exist" );
		}

		void flush( )
		{
			if( parent_->properties( ).get_property_with_key( key_redirect_ ).value< int >( ) )
				prop_stdout_.set( prop_stdout_.value< std::string >( ) + output_->str( ) );
			else
				std::cout << output_->str( );
			output_->str( "" );
		}

		void run( sequence_ptr seq )
		{
			execution_stack_.push_back( seq );
			while( !execution_stack_.back( )->done( ) )
			{
				push( execution_stack_.back( )->next( ) );
				flush( );
			}
			execution_stack_.pop_back( );
		}

		void run( const std::vector< pl::wstring > &words, bool parseable = false )
		{
			run( sequence_ptr( new sequence( words, parseable ) ) );
		}

		void run( const pl::wstring_list &words, bool parseable = false )
		{
			run( sequence_ptr( new sequence( words, parseable ) ) );
		}

		void execute_word( pl::wstring name )
		{
			const std::vector< pl::wstring > &function = words_[ name ];
			if ( function.size( ) )
			{
				variables_push( );
				run( function );
				variables_pop( );
			}
		}

		void execute_inline( const pl::wstring &name )
		{
			const std::vector< pl::wstring > &function = inline_[ name ];
			run( function );
		}

		void execute_operation( const pl::wstring &name )
		{
			if ( state_ == 0 && !empty( ) ) push( pop( ) );
			if ( next_op_.size( ) == 0 )
			{
				aml_operation op = operations_[ name ];
				op.run( this );
			}
			else
			{
				operation op = next_op_.back( );
				inputs_.push_back( ml::input_type_ptr( new input_value( name ) ) );
				op( this );
			}
		}

		void create_filter( const std::wstring &arg )
		{
			if ( !empty( ) ) push( pop( ) );
			ml::filter_type_ptr filter = ml::create_filter( arg.substr( 7 ) );
			if ( filter )
			{
				filter->report_exceptions( parent_->reporting_exceptions( ) );
				inputs_.push_back( filter );
			}
			else
			{
				throw std::string( "failed to create " ) + pl::to_string( arg );
			}
		}

		void create_input( const std::wstring &arg )
		{
			if ( !empty( ) ) push( pop( ) );
			ml::input_type_ptr input = ml::create_delayed_input( arg );
			if ( input )
			{
				input->report_exceptions( parent_->reporting_exceptions( ) );
				inputs_.push_back( input );
			}
			else
			{
				throw std::string( "failed to create input " ) + pl::to_string( arg );
			}
		}

		void create_int( const pl::wstring &arg )
		{
			if ( !empty( ) ) push( pop( ) );
			boost::int64_t result = atoll( pl::to_string( arg ).c_str( ) );
			inputs_.push_back( ml::input_type_ptr( new stack_value< boost::int64_t >( result ) ) );
		}

		void create_double( const pl::wstring &arg )
		{
			if ( !empty( ) ) push( pop( ) );
			double result = atof( pl::to_string( arg ).c_str( ) );
			inputs_.push_back( ml::input_type_ptr( new stack_value< double >( result ) ) );
		}

		void create_numeric( const pl::wstring &arg )
		{
			if ( !empty( ) ) push( pop( ) );
			inputs_.push_back( ml::input_type_ptr( new input_value( arg ) ) );
		}

		ml::input_type_ptr pop( )
		{
			ml::input_type_ptr result;

			if ( state_ != 0 )
				throw std::string( "Syntax error - unclosed function" );

			if ( inputs_.size( ) )
			{
				result = inputs_.back( );
				inputs_.pop_back( );
				if ( result && ( !result->property_with_key( key_aml_value_ ).valid( ) && is_local( result->get_uri( ) ) ) )
				{
					;
				}
				else if ( result && result->init( ) )
				{
					int slots = static_cast<int>(result->slot_count( ));
					while( slots )
					{
						if ( result->fetch_slot( -- slots ) == 0 )
						{
							ml::input_type_ptr slot = pop( );
							if ( slot ) // && slot->get_frames( ) >= 0 )
								result->connect( slot, slots );
							else
								throw std::string( "Unresolved filter or input" );
						}
					}
				}
				else
				{
					throw std::string( "Unresolved filter or input" );
				}
			}
			else
			{
				throw std::string( "Stack underflow" );
			}

			return result;
		}

		double pop_value( )
		{
			double result = 0.0;

			if ( inputs_.size( ) )
			{
				ml::input_type_ptr value = inputs_.back( );
				inputs_.pop_back( );
				pl::pcos::property prop = value->property_with_key( key_aml_value_ );
				if ( prop.valid( ) && prop.is_a< double >( ) )
					result = prop.value< double >( );
				else if ( prop.valid( ) && prop.is_a< int >( ) )
					result = double( prop.value< int >( ) );
				else if ( prop.valid( ) && prop.is_a< boost::int64_t >( ) )
					result = double( prop.value< boost::int64_t >( ) );
				else
					result = atof( pl::to_string( value->get_uri( ) ).c_str( ) );
			}
			else
			{
				throw std::string( "Stack underflow" );
			}

			return result;
		}

		template < typename T > T pop_numeric( )
		{
			T result;

			if ( inputs_.size( ) )
			{
				ml::input_type_ptr value = inputs_.back( );
				inputs_.pop_back( );
				pl::pcos::property prop = value->property_with_key( key_aml_value_ );
				if ( prop.valid( ) && prop.is_a< double >( ) )
					result = T( prop.value< double >( ) );
				else if ( prop.valid( ) && prop.is_a< int >( ) )
					result = T( prop.value< int >( ) );
				else if ( prop.valid( ) && prop.is_a< boost::int64_t >( ) )
					result = T( prop.value< boost::int64_t >( ) );
				else
					result = T( atof( pl::to_string( value->get_uri( ) ).c_str( ) ) );
			}
			else
			{
				throw std::string( "Stack underflow" );
			}

			return result;
		}

		void debug( const std::string msg )
		{
			*output_ << msg << ": ";
			for ( std::deque < ml::input_type_ptr >::iterator iter = inputs_.begin( ); iter != inputs_.end( ); iter ++ )
				*output_ << "\"" << pl::to_string( ( *iter )->get_uri( ) ) << "\" ";
			*output_ << std::endl;
			flush( );
		}

		void rdebug( const std::string msg )
		{
			*output_ << msg << ": ";
			for ( std::deque < ml::input_type_ptr >::iterator iter = rstack_.begin( ); iter != rstack_.end( ); iter ++ )
				*output_ << "\"" << pl::to_string( ( *iter )->get_uri( ) ) << "\" ";
			*output_ << std::endl;
			flush( );
		}

		bool is_an_aml_script( const pl::wstring &filename )
		{
           	olib::t_path apath( cl::str_util::to_t_string(filename));
           	/// TODO: Fix this so filename is opened via olib::fs_t_ifstream instead            
           	std::ifstream file(  cl::str_util::to_string(apath.string()).c_str() , std::ifstream::in );

			if ( !file.is_open( ) )
				return false;

			std::string sub1;
			std::string sub2;
			file >> sub1 >> sub2;

			return sub1.find( "#!" ) == 0 && sub2.find( "amlbatch" ) != std::string::npos;
		}

		void parse_file( const pl::wstring &filename )
		{
			if ( filename != L"stdin:" )
			{
            	olib::t_path apath( cl::str_util::to_t_string(filename));
            	/// TODO: Fix this so filename is opened via olib::fs_t_ifstream instead            
            	std::ifstream file(  cl::str_util::to_string(apath.string()).c_str() , std::ifstream::in );

				paths_.push_back( fs::system_complete( apath ).parent_path( ) );

				if ( !file.is_open( ) )
					throw std::string( "Unable to find " ) + pl::to_string( filename );

            	parse_stream( file );

				paths_.pop_back( );
			}
			else
			{
				paths_.push_back( fs::system_complete( _CT( "" ) ).parent_path( ) );
            	parse_stream( std::cin );
				paths_.pop_back( );
			}
		}

		void parse_http( const olib::t_string &url )
		{
			throw std::string( "Unable to access http at this point" );
		}

		void parse_stream( std::istream &file )
		{
			std::string token;
			std::vector< pl::wstring > tokens;
			std::string line;

			while ( std::getline( file, line ) ) 
			{
				std::istringstream iss;
				iss.str( line );

				while( iss.good( ) && !iss.eof( ) )
				{
					std::string sub;
					iss >> std::skipws >> sub;

					if ( token == "" && sub[ 0 ] == '#' )
						break;

					if ( token != "" )
						token += std::string( " " ) + sub;
					else
						token = sub;

					while ( iss.good( ) && !iss.eof( ) && count_quotes( token ) % 2 != 0 )
					{
						char t;
						iss >> std::noskipws >> t;
						token += t;
					}

					if ( token != "" )
					{
						tokens.push_back( pl::to_wstring( token ) );
						token = "";
					}
				}

				if ( token == "" )
				{
					run( tokens, true );
					tokens.erase( tokens.begin( ), tokens.end( ) );
				}
			}

			run( tokens, true );
		}

		bool is_file( const olib::t_path& path, const olib::t_string& token )
		{
			return !( boost::regex_match( token, aml_syntax ) || 
					  boost::regex_match( token, prop_syntax ) ) && 
					  fs::exists( olib::t_path( path / token ) );
		}

		inline int count_quotes( const std::string &token ) const
		{
			int quotes = 0;
			for ( size_t i = 0; i < token.size( ); i ++ )
				if ( token[ i ] == '\"' )
					quotes ++;
			return quotes;
		}

		ml::input_type *parent_;
		pl::pcos::property &prop_stdout_;
		std::ostringstream *output_;
		std::deque < ml::input_type_ptr > inputs_;
		std::deque < ml::input_type_ptr > rstack_;
		std::deque < olib::t_path > paths_;
		std::map < pl::wstring, aml_operation > operations_;
		std::map < pl::wstring, std::vector< pl::wstring > > words_;
		std::map < pl::wstring, std::vector< pl::wstring > > inline_;
		std::vector< pl::wstring > loops_;
		std::vector < pl::wstring > word_;
		std::vector < std::map< pl::wstring, ml::input_type_ptr > > variables_;
		int word_count_;
		std::deque < bool > conds_;
		int state_;
		int loop_count_;
		int trace_;
		std::vector< operation > next_op_;
		std::vector< sequence_ptr > execution_stack_;
		std::vector< bool > loop_cond_;
		operation next_exec_op_;
		int ignore_;
};

static void mc_workaround( aml_stack *stack )
{
	ml::input_type_ptr colour_input = ml::create_input( "colour:" );
	ml::store_type_ptr dummy = ml::create_store( "mc:high:mc_workaround_dummy.ts", colour_input->fetch() );
}

static void op_dot( aml_stack *stack )
{
	ml::input_type_ptr tos = stack->pop( );
	tos->sync( );
	stack->parent_->connect( tos );
	stack->parent_->property( "threaded" ) = int( contains_sub_thread( tos ) );
}

static void op_colon( aml_stack *stack )
{
	if ( stack->state_ == 0 )
	{
		stack->state_ = 1;
		stack->word_count_ = 1;
	}
	else if ( stack->state_ != 0 )
	{
		stack->word_count_ ++;
		stack->word_.push_back( L":" );
	}
}

static void op_semi_colon( aml_stack *stack )
{
	if ( stack->state_ == 1 && stack->word_count_ == 1 )
	{
		stack->commit_word( );
		stack->word_count_ --;
		stack->state_ = 0;
	}
	else if ( stack->word_count_ > 1 )
	{
		stack->word_count_ --;
		stack->word_.push_back( L";" );
	}
	else
	{
		throw std::string( "Badly formed word definition" );
	}
}

static void op_str( aml_stack *stack )
{
	if ( stack->next_op( op_str ) )
	{
		ml::input_type_ptr word = stack->pop( );
		if ( word->get_uri( ).find( L"%s" ) != pl::wstring::npos )
		{
			pl::wstring fmt = word->get_uri( );
			pl::wstring::size_type pos = 0;
			std::vector < pl::wstring > tokens;
			while( pos != pl::wstring::npos )
			{
				pos = fmt.find( L"%s", pos );
				if ( pos != pl::wstring::npos )
				{
					tokens.push_back( stack->pop( )->get_uri( ) );
					pos += 2;
				}
			}

			boost::format fmtr( pl::to_string( fmt ) );
			while( tokens.size( ) )
			{
				fmtr % pl::to_string( tokens.back( ) );
				tokens.pop_back( );
			}

			stack->push( ml::input_type_ptr( new input_value( pl::to_wstring( fmtr.str( ) ) ) ) );
		}
		else
		{
			stack->push( word );
		}
	}
}

static void op_handler( aml_stack *stack )
{
	stack->next_op( op_handler );
	stack->handled( L"" );
}

static void op_parse( aml_stack *stack )
{
	if ( !stack->next_op( op_parse ) )
	{
		bool found = false;

		if ( stack->execution_stack_.size( ) )
		{
			bool hosting_extension = stack->execution_stack_.back( ) == dummy_sequence_;

			for ( std::vector< sequence_ptr >::iterator iter = -- stack->execution_stack_.end( ); !found && iter != stack->execution_stack_.begin( ); )
			{
				found = ( *( -- iter ) )->is_parseable( ) || hosting_extension;
				if ( found )
				{
					if ( !( *iter )->done( ) )
					{
						pl::wstring token = ( *iter )->next( );
						if ( token != L"parse" )
							stack->push( token );
						else
							found = false;
					}
					else
						stack->push( L"" );
				}
			}
		}

		if ( !found )
		{
			stack->parent_->property( "query" ) = pl::wstring( L"parse-token" );
			stack->push( stack->parent_->property( "token" ).value< pl::wstring >( ) );
		}
	}
}

static void op_parseable( aml_stack *stack )
{
	if ( stack->execution_stack_.size( ) )
		stack->execution_stack_.back( )->parseable( true );
}

static void op_throw( aml_stack *stack )
{
	if ( stack->next_op( op_throw ) )
	{
		pl::wstring word = stack->pop( )->get_uri( );
		throw std::string( pl::to_string( word ) );
	}
}

static void op_execute_sequence( aml_stack *stack, ml::input_type_ptr input )
{
	if ( input->slot_count( ) && input->get_frames( ) == 0 )
	{
		for ( size_t i = 0; i < input->slot_count( ); i ++ )
			stack->push( input->fetch_slot( i )->get_uri( ) );
	}
	else
	{
		stack->push( input );
		throw std::string( "Attempt to execute an invalid sequence" );
	}
}

static void op_execute( aml_stack *stack )
{
	ml::input_type_ptr input = stack->pop( );
	if ( input->slot_count( ) )
	{
		op_execute_sequence( stack, input );
	}
	else
	{
		if ( stack->next_exec_op_ == 0 )
		{
			pl::wstring word = input->get_uri( );
			stack->push( word );
		}
		else
		{
			stack->inputs_.push_back( input );
			stack->next_op( stack->next_exec_op_ );
			stack->next_exec_op_( stack );
		}

		if ( stack->next_op_.size( ) )
		{
			stack->next_exec_op_ = stack->next_op_.back( );
			stack->next_op_.pop_back( );
		}
		else
		{
			stack->next_exec_op_ = 0;
		}
	}
}

static void op_forget( aml_stack *stack )
{
	if ( stack->next_op( op_forget ) )
	{
		pl::wstring word = stack->pop( )->get_uri( );
		stack->forget_word( word );
	}
}

static void op_plus( aml_stack *stack )
{
	double a = stack->pop_value( );
	double b = stack->pop_value( );
	stack->push( b + a );
}

static void op_minus( aml_stack *stack )
{
	double a = stack->pop_value( );
	double b = stack->pop_value( );
	stack->push( b - a );
}

static void op_div( aml_stack *stack )
{
	double a = stack->pop_value( );
	double b = stack->pop_value( );
	stack->push( b / a );
}

static void op_mult( aml_stack *stack )
{
	double a = stack->pop_value( );
	double b = stack->pop_value( );
	stack->push( b * a );
}

static void op_pow( aml_stack *stack )
{
	double a = stack->pop_value( );
	double b = stack->pop_value( );
	stack->push( std::pow( b, a ) );
}

static void op_mod( aml_stack *stack )
{
	double a = stack->pop_value( );
	double b = stack->pop_value( );
	stack->push( std::fmod( b, a ) );
}

static void op_abs( aml_stack *stack )
{
	double a = stack->pop_value( );
	stack->push( std::fabs( a ) );
}

static void op_floor( aml_stack *stack )
{
	double a = stack->pop_value( );
	stack->push( std::floor( a ) );
}

static void op_ceil( aml_stack *stack )
{
	double a = stack->pop_value( );
	stack->push( std::ceil( a ) );
}

static void op_sin( aml_stack *stack )
{
	double a = stack->pop_value( );
	stack->push( std::sin( a ) );
}

static void op_cos( aml_stack *stack )
{
	double a = stack->pop_value( );
	stack->push( std::cos( a ) );
}

static void op_tan( aml_stack *stack )
{
	double a = stack->pop_value( );
	stack->push( std::tan( a ) );
}

static void op_prop_query_parse( aml_stack *stack )
{
	if ( stack->next_op( op_prop_query_parse ) )
	{
		ml::input_type_ptr a = stack->pop( );
		stack->push_property( a->get_uri( ) );
	}
}

static void op_has_prop_parse( aml_stack *stack )
{
	if ( stack->next_op( op_has_prop_parse ) )
	{
		ml::input_type_ptr a = stack->pop( );
		ml::input_type_ptr b = stack->pop( );
		stack->push( b );
		std::string name = pl::to_string( a->get_uri( ) );
		stack->push( b->property( name.c_str( ) ).valid( ) );
	}
}

static void op_prop_query( aml_stack *stack )
{
	ml::input_type_ptr a = stack->pop( );
	stack->push_property( a->get_uri( ) );
}

static void op_prop_exists( aml_stack *stack )
{
	ml::input_type_ptr a = stack->pop( );
	ml::input_type_ptr b = stack->pop( );
	stack->push( b );
	std::string name = pl::to_string( a->get_uri( ) );
	stack->push( b->property( name.c_str( ) ).valid( ) );
}

static void op_prop_matches( aml_stack *stack )
{
	ml::input_type_ptr a = stack->pop( );
	ml::input_type_ptr b = stack->pop( );
	stack->push( b );
	std::string name = pl::to_string( a->get_uri( ) );
	boost::regex match( name );
	pl::pcos::key_vector keys = b->properties( ).get_keys( );
	int count = 0;
	for( pl::pcos::key_vector::iterator it = keys.begin( ); it != keys.end( ); it ++ )
	{
		if ( boost::regex_match( ( *it ).as_string( ), match ) )
		{
			stack->push( ml::input_type_ptr( new input_value( pl::to_wstring( ( *it ).as_string( ) ) ) ) );
			count ++;
		}
	}
	stack->push( double( count ) );
}

static void op_length( aml_stack *stack )
{
	ml::input_type_ptr a = stack->pop( );
	stack->push( a );
	a->sync( );
	stack->push( double( a->get_frames( ) ) );
}

static void op_eval( aml_stack *stack )
{
	ml::input_type_ptr a = stack->pop( );
	stack->push( a );
}

static void op_drop( aml_stack *stack )
{
	stack->pop( );
}

static void op_pick( aml_stack *stack )
{
	int a = int( stack->pop_value( ) );
	if ( a >= 0 && a < int( stack->inputs_.size( ) ) )
	{
		std::deque < ml::input_type_ptr >::iterator b = stack->inputs_.end( ) - a - 1;
		stack->push( *b );
	}
	else
	{
		throw std::string( "Stack underflow" );
	}
}

static void op_rpick( aml_stack *stack )
{
	int a = int( stack->pop_value( ) );
	if ( a >= 0 && a < int( stack->rstack_.size( ) ) )
	{
		std::deque < ml::input_type_ptr >::iterator b = stack->rstack_.end( ) - a - 1;
		stack->push( *b );
	}
	else
	{
		throw std::string( "Return stack underflow" );
	}
}

static void op_rroll( aml_stack *stack )
{
	int a = int( stack->pop_value( ) );
	if ( a >= 0 && a < int( stack->rstack_.size( ) ) )
	{
		std::deque < ml::input_type_ptr >::iterator b = stack->rstack_.end( ) - a - 1;
		stack->push( *b );
		stack->rstack_.erase( b );
	}
	else
	{
		throw std::string( "Return stack underflow" );
	}
}

static void op_roll( aml_stack *stack )
{
	int a = int( stack->pop_value( ) );
	if ( a >= 0 && a < int( stack->inputs_.size( ) ) )
	{
		std::deque < ml::input_type_ptr >::iterator b = stack->inputs_.end( ) - a - 1;
		ml::input_type_ptr temp = *b;
		stack->inputs_.erase( b );
		stack->push( temp );
	}
	else
	{
		throw std::string( "Stack underflow" );
	}
}

static void op_shift( aml_stack *stack )
{
	int a = int( stack->pop_value( ) );
	if ( a >= 0 && a < int( stack->inputs_.size( ) ) )
	{
		ml::input_type_ptr b = stack->pop( );
		std::deque < ml::input_type_ptr >::iterator c = stack->inputs_.end( ) - a;
		stack->inputs_.insert( c, b );
	}
	else
	{
		throw std::string( "Stack underflow" );
	}
}

static void op_rpush( aml_stack *stack )
{
	ml::input_type_ptr a = stack->pop( );
	stack->rstack_.push_back( a );
}

static void op_rpop( aml_stack *stack )
{
	if ( stack->rstack_.size( ) > 0 )
	{
		ml::input_type_ptr a = stack->rstack_.back( );
		stack->rstack_.pop_back( );
		stack->push( a );
	}
	else
	{
		throw std::string( "Return stack underflow" );
	}
}

static void op_rdup( aml_stack *stack )
{
	if ( stack->rstack_.size( ) > 0 )
	{
		ml::input_type_ptr a = stack->rstack_.back( );
		stack->push( a );
	}
	else
	{
		throw std::string( "Return stack underflow" );
	}
}

static void op_rdrop( aml_stack *stack )
{
	if ( stack->rstack_.size( ) > 0 )
	{
		stack->rstack_.pop_back( );
	}
	else
	{
		throw std::string( "Return stack underflow" );
	}
}

static void op_rdepth( aml_stack *stack )
{
	stack->push( double( stack->rstack_.size( ) ) );
}

struct ml_query_traits : public pl::default_query_traits
{
	ml_query_traits( const pl::wstring &type )
		: type_( type )
	{ }
		
	pl::wstring libname( ) const
	{ return pl::wstring( L"openmedialib" ); }

	pl::wstring type( ) const
	{ return pl::wstring( type_ ); }

	const pl::wstring type_;
};

static void query_type( aml_stack *stack, pl::wstring type )
{
	typedef pl::discovery< ml_query_traits > discovery;
	ml_query_traits query( type );
	discovery plugins( query );

	( *stack->output_ ) << pl::to_string( type ) << ":" << std::endl << std::endl;

	for ( discovery::const_iterator i = plugins.begin( ); i != plugins.end( ); i ++ )
	{
		std::vector< pl::wstring > files = ( *i ).filename( );
		std::vector< pl::wstring > contents = ( *i ).extension( );
		bool found = false;

		for ( std::vector< pl::wstring >::iterator f = files.begin( ); !found && f != files.end( ); f ++ )
		{
			if ( fs::exists( pl::to_string( *f ) ) )
			{
				( *stack->output_ ) << pl::to_string( *f ) << " : ";
				found = true;
			}
		}

		if ( found )
		{
			for ( std::vector< pl::wstring >::iterator j = contents.begin( ); j != contents.end( ); j ++ )
			{
				( *stack->output_ ) << pl::to_string( *j ) << " ";
			}
			( *stack->output_ ) << std::endl;
		}
	}
	( *stack->output_ ) << std::endl;
}

static void op_available( aml_stack *stack )
{
	query_type( stack, pl::wstring( L"input" ) );
	query_type( stack, pl::wstring( L"filter" ) );
	query_type( stack, pl::wstring( L"output" ) );
}

static void op_dump( aml_stack *stack )
{
	stack->debug( "dump" );
}

static void op_rdump( aml_stack *stack )
{
	stack->rdebug( "rdump" );
}

static void op_tos( aml_stack *stack )
{
	ml::input_type_ptr a = stack->pop( );
	*( stack->output_ ) << pl::to_string( a->get_uri( ) ) << std::endl;
	stack->push( a );
}

static void op_trace( aml_stack *stack )
{
	int level = int( stack->pop_value( ) );
	stack->set_trace( level );
}

static void op_words( aml_stack *stack )
{
	stack->print_words( );
}

static void op_dict( aml_stack *stack )
{
	stack->dict( );
}

static void op_if( aml_stack *stack )
{
	if ( stack->conds_.back( ) )
	{
		int a = int( stack->pop_value( ) );
		stack->conds_.push_back( a != 0 );
	}
	else
	{
		stack->conds_.push_back( false );
	}
}

static void op_else( aml_stack *stack )
{
	if ( *( stack->conds_.end( ) - 2 ) )
	{
		bool current = stack->conds_.back( );
		stack->conds_.pop_back( );
		stack->conds_.push_back( !current );
	}
}

static void op_then( aml_stack *stack )
{
	stack->conds_.pop_back( );
}

static void op_equal( aml_stack *stack )
{
	ml::input_type_ptr a = stack->pop( );
	ml::input_type_ptr b = stack->pop( );
	if ( a != b )
	{
		pl::pcos::property prop_a = a->property_with_key( key_aml_value_ );
		pl::pcos::property prop_b = b->property_with_key( key_aml_value_ );
		if ( prop_a.valid( ) && prop_b.valid( ) )
		{
			bool done = true;
			if ( prop_a.is_a< boost::int64_t >( ) && prop_b.is_a< boost::int64_t >( ) )
				stack->push( prop_a.value< boost::int64_t >( ) == prop_b.value< boost::int64_t >( ) );
			else if ( prop_a.is_a< int >( ) && prop_b.is_a< int >( ) )
				stack->push( prop_a.value< int >( ) == prop_b.value< int >( ) );
			else if ( prop_a.is_a< double >( ) && prop_b.is_a< double >( ) )
				stack->push( prop_a.value< double >( ) == prop_b.value< double >( ) );
			else
				done = false;
			if ( done )
				return;
		}

		if ( boost::regex_match( pl::to_string( a->get_uri( ) ), numeric_syntax ) && 
			 boost::regex_match( pl::to_string( b->get_uri( ) ), numeric_syntax ) )
		{
			double va = atof( pl::to_string( a->get_uri( ) ).c_str( ) );
			double vb = atof( pl::to_string( b->get_uri( ) ).c_str( ) );
			stack->push( vb == va );
		}
		else
		{
			stack->push( b->get_uri( ) == a->get_uri( ) );
		}
	}
	else
	{
		stack->push( 1 );
	}
}

static void op_is( aml_stack *stack )
{
	ml::input_type_ptr a = stack->pop( );
	ml::input_type_ptr b = stack->pop( );
	stack->push( a == b );
}

static void op_lt( aml_stack *stack )
{
	ml::input_type_ptr a = stack->pop( );
	ml::input_type_ptr b = stack->pop( );
	pl::pcos::property prop_b = b->property_with_key( key_aml_value_ );
	pl::pcos::property prop_a = a->property_with_key( key_aml_value_ );

	if ( prop_a.valid( ) && prop_b.valid( ) )
	{
		bool done = true;
		if ( prop_b.is_a< boost::int64_t >( ) && prop_a.is_a< boost::int64_t >( ) )
			stack->push( prop_b.value< boost::int64_t >( ) < prop_a.value< boost::int64_t >( ) );
		else if ( prop_b.is_a< int >( ) && prop_a.is_a< int >( ) )
			stack->push( prop_b.value< int >( ) < prop_a.value< int >( ) );
		else if ( prop_b.is_a< double >( ) && prop_a.is_a< double >( ) )
			stack->push( prop_b.value< double >( ) < prop_a.value< double >( ) );
		else
			done = false;
		if ( done )
			return;
	}

	if ( boost::regex_match( pl::to_string( a->get_uri( ) ), numeric_syntax ) && 
		 boost::regex_match( pl::to_string( b->get_uri( ) ), numeric_syntax ) )
	{
		double va = atof( pl::to_string( a->get_uri( ) ).c_str( ) );
		double vb = atof( pl::to_string( b->get_uri( ) ).c_str( ) );
		stack->push( vb < va );
	}
	else
	{
		stack->push( b->get_uri( ) < a->get_uri( ) );
	}
}

static void op_gt( aml_stack *stack )
{
	ml::input_type_ptr a = stack->pop( );
	ml::input_type_ptr b = stack->pop( );
	pl::pcos::property prop_b = b->property_with_key( key_aml_value_ );
	pl::pcos::property prop_a = a->property_with_key( key_aml_value_ );

	if ( prop_a.valid( ) && prop_b.valid( ) )
	{
		bool done = true;
		if ( prop_b.is_a< boost::int64_t >( ) && prop_a.is_a< boost::int64_t >( ) )
			stack->push( prop_b.value< boost::int64_t >( ) > prop_a.value< boost::int64_t >( ) );
		else if ( prop_b.is_a< int >( ) && prop_a.is_a< int >( ) )
			stack->push( prop_b.value< int >( ) > prop_a.value< int >( ) );
		else if ( prop_b.is_a< double >( ) && prop_a.is_a< double >( ) )
			stack->push( prop_b.value< double >( ) > prop_a.value< double >( ) );
		else
			done = false;
		if ( done )
			return;
	}

	if ( boost::regex_match( pl::to_string( a->get_uri( ) ), numeric_syntax ) && 
		 boost::regex_match( pl::to_string( b->get_uri( ) ), numeric_syntax ) )
	{
		double va = atof( pl::to_string( a->get_uri( ) ).c_str( ) );
		double vb = atof( pl::to_string( b->get_uri( ) ).c_str( ) );
		stack->push( vb > va );
	}
	else
	{
		stack->push( b->get_uri( ) > a->get_uri( ) );
	}
}

static void op_lt_equal( aml_stack *stack )
{
	ml::input_type_ptr a = stack->pop( );
	ml::input_type_ptr b = stack->pop( );
	pl::pcos::property prop_b = b->property_with_key( key_aml_value_ );
	pl::pcos::property prop_a = a->property_with_key( key_aml_value_ );

	if ( prop_a.valid( ) && prop_b.valid( ) )
	{
		bool done = true;
		if ( prop_b.is_a< boost::int64_t >( ) && prop_a.is_a< boost::int64_t >( ) )
			stack->push( prop_b.value< boost::int64_t >( ) <= prop_a.value< boost::int64_t >( ) );
		else if ( prop_b.is_a< int >( ) && prop_a.is_a< int >( ) )
			stack->push( prop_b.value< int >( ) <= prop_a.value< int >( ) );
		else if ( prop_b.is_a< double >( ) && prop_a.is_a< double >( ) )
			stack->push( prop_b.value< double >( ) <= prop_a.value< double >( ) );
		else
			done = false;
		if ( done )
			return;
	}

	if ( boost::regex_match( pl::to_string( a->get_uri( ) ), numeric_syntax ) && 
		 boost::regex_match( pl::to_string( b->get_uri( ) ), numeric_syntax ) )
	{
		double va = atof( pl::to_string( a->get_uri( ) ).c_str( ) );
		double vb = atof( pl::to_string( b->get_uri( ) ).c_str( ) );
		stack->push( vb <= va );
	}
	else
	{
		stack->push( b->get_uri( ) <= a->get_uri( ) );
	}
}

static void op_gt_equal( aml_stack *stack )
{
	ml::input_type_ptr a = stack->pop( );
	ml::input_type_ptr b = stack->pop( );
	pl::pcos::property prop_b = b->property_with_key( key_aml_value_ );
	pl::pcos::property prop_a = a->property_with_key( key_aml_value_ );

	if ( prop_a.valid( ) && prop_b.valid( ) )
	{
		bool done = true;
		if ( prop_b.is_a< boost::int64_t >( ) && prop_a.is_a< boost::int64_t >( ) )
			stack->push( prop_b.value< boost::int64_t >( ) >= prop_a.value< boost::int64_t >( ) );
		else if ( prop_b.is_a< int >( ) && prop_a.is_a< int >( ) )
			stack->push( prop_b.value< int >( ) >= prop_a.value< int >( ) );
		else if ( prop_b.is_a< double >( ) && prop_a.is_a< double >( ) )
			stack->push( prop_b.value< double >( ) >= prop_a.value< double >( ) );
		else
			done = false;
		if ( done )
			return;
	}

	if ( boost::regex_match( pl::to_string( a->get_uri( ) ), numeric_syntax ) && 
		 boost::regex_match( pl::to_string( b->get_uri( ) ), numeric_syntax ) )
	{
		double va = atof( pl::to_string( a->get_uri( ) ).c_str( ) );
		double vb = atof( pl::to_string( b->get_uri( ) ).c_str( ) );
		stack->push( vb >= va );
	}
	else
	{
		stack->push( b->get_uri( ) >= a->get_uri( ) );
	}
}

static void op_begin( aml_stack *stack )
{
	if ( stack->loop_count_ == 0 )
		stack->state_ = 2;
	else
		stack->loops_.push_back( L"begin" );
	stack->loop_count_ ++;
}

static void op_until( aml_stack *stack )
{
	if ( -- stack->loop_count_ == 0 )
	{
		stack->state_ = 0;
		sequence_ptr seq = sequence_ptr( new sequence( stack->loops_ ) );
		stack->loops_.erase( stack->loops_.begin( ), stack->loops_.end( ) );
		do
		{
			seq->start( );
			stack->run( seq );
		}
		while( !stack->pop_value( ) );
	}
	else
	{
		stack->loops_.push_back( L"until" );
	}
}

static void op_while( aml_stack *stack )
{
	stack->loop_cond_.push_back( int( stack->pop_value( ) ) != 0 );
	if ( !stack->loop_cond_.back( ) )
		stack->execution_stack_.back( )->end( );
}

static void op_repeat( aml_stack *stack )
{
	if ( -- stack->loop_count_ == 0 )
	{
		bool running = true;
		stack->state_ = 0;

		sequence_ptr seq = sequence_ptr( new sequence( stack->loops_ ) );
		stack->loops_.erase( stack->loops_.begin( ), stack->loops_.end( ) );

		do
		{
			seq->start( );
			stack->run( seq );
			running = stack->loop_cond_.back( );
			stack->loop_cond_.pop_back( );
		}
		while( running );
	}
	else
	{
		stack->loops_.push_back( L"repeat" );
	}
}

static void op_pack( aml_stack *stack )
{
	int count = stack->pop_numeric< int >( );
	ml::filter_type_ptr filter = ml::create_filter( L"playlist" );
	filter->property_with_key( key_slots_ ) = count;
	stack->push( filter );
}

static void op_pack_append( aml_stack *stack )
{
	ml::input_type_ptr value = stack->pop( );
	ml::input_type_ptr list = stack->pop( );
	stack->push( list );
	list->property_with_key( key_slots_ ) = int( list->slot_count( ) + 1 );
	list->connect( value, size_t( list->slot_count( ) - 1 ) );
}

static void op_pack_append_list( aml_stack *stack )
{
	ml::input_type_ptr value = stack->pop( );
	ml::input_type_ptr list = stack->pop( );
	stack->push( list );
	size_t dst_index = list->slot_count( );
	list->property_with_key( key_slots_ ) = int( list->slot_count( ) + value->slot_count( ) );
	for ( size_t src_index = 0; src_index <  value->slot_count( ); src_index ++, dst_index ++ )
		list->connect( value->fetch_slot( src_index ), dst_index );
}

static void op_pack_insert( aml_stack *stack )
{
	int offset = stack->pop_numeric< int >( );
	ml::input_type_ptr value = stack->pop( );
	ml::input_type_ptr list = stack->pop( );
	stack->push( list );
	if ( offset < 0 )
		offset = list->slot_count( ) + offset;
	if ( offset >= 0 && offset <= int( list->slot_count( ) ) )
	{
		list->property_with_key( key_slots_ ) = int( list->slot_count( ) + 1 );
		for ( int i = int( list->slot_count( ) ) - 2; i >= ( offset ); i -- )
			list->connect( list->fetch_slot( size_t( i ) ), size_t( i + 1 ) );
		list->connect( value, offset );
	}
	else
	{
		throw std::string( "Index out of range on pack insert" );
	}
}

static void op_pack_remove( aml_stack *stack )
{
	int offset = stack->pop_numeric< int >( );
	ml::input_type_ptr list = stack->pop( );
	stack->push( list );
	if ( offset < 0 )
		offset = list->slot_count( ) + offset;
	if ( offset >= 0 && offset < int( list->slot_count( ) ) )
	{
		for ( size_t i = size_t( offset ); i < list->slot_count( ); i ++ )
			list->connect( list->fetch_slot( i + 1 ), i );
		list->property_with_key( key_slots_ ) = int( list->slot_count( ) - 1 );
	}
	else
	{
		throw std::string( "Index out of range on pack remove" );
	}
}

static void op_pack_query( aml_stack *stack )
{
	int offset = stack->pop_numeric< int >( );
	ml::input_type_ptr list = stack->pop( );
	stack->push( list );
	if ( offset < 0 )
		offset = list->slot_count( ) + offset;
	if ( offset >= 0 && offset < int( list->slot_count( ) ) )
		stack->push( list->fetch_slot( size_t( offset ) ) );
	else
		throw std::string( "Index out of range on pack query" );
}

static void op_pack_assign( aml_stack *stack )
{
	int offset = stack->pop_numeric< int >( );
	ml::input_type_ptr value = stack->pop( );
	ml::input_type_ptr list = stack->pop( );
	stack->push( list );
	if ( offset < 0 )
		offset = list->slot_count( ) + offset;
	if ( offset >= 0 && offset < int( list->slot_count( ) ) )
		list->connect( value, size_t( offset ) );
	else
		throw std::string( "Index out of range on pack query" );
}

static void op_iter_start( aml_stack *stack )
{
	if ( stack->loop_count_ == 0 )
		stack->state_ = 2;
	else
		stack->loops_.push_back( L"iter_start" );
	stack->loop_count_ ++;
}

static void op_iter_slots( aml_stack *stack )
{
	if ( -- stack->loop_count_ == 0 )
	{
		stack->state_ = 0;
		ml::input_type_ptr a = stack->pop( );

		sequence_ptr seq = sequence_ptr( new sequence( stack->loops_ ) );
		stack->loops_.erase( stack->loops_.begin( ), stack->loops_.end( ) );

		for ( size_t i = 0; i < a->slot_count( ); i ++ )
		{
			seq->start( );
			stack->push( a->fetch_slot( i ) );
			stack->run( seq );
		}

		stack->push( a );
	}
	else
	{
		stack->loops_.push_back( L"iter_slots" );
	}
}

static void op_iter_frames( aml_stack *stack )
{
	if ( -- stack->loop_count_ == 0 )
	{
		stack->state_ = 0;
		ml::input_type_ptr a = stack->pop( );

		sequence_ptr seq = sequence_ptr( new sequence( stack->loops_ ) );
		stack->loops_.erase( stack->loops_.begin( ), stack->loops_.end( ) );
		a->sync( );

		for ( int i = 0; i < a->get_frames( ); i ++ )
		{
			seq->start( );
			ml::frame_type_ptr f = a->fetch( i );
			stack->push( ml::input_type_ptr( new frame_value( f, a ) ) );
			stack->run( seq );
		}

		stack->push( a );
	}
	else
	{
		stack->loops_.push_back( L"iter_frames" );
	}
}

static void op_iter_range( aml_stack *stack )
{
	if ( -- stack->loop_count_ == 0 )
	{
		stack->state_ = 0;
		ml::input_type_ptr a = stack->pop( );
		boost::int64_t value;

		pl::pcos::property prop = a->property_with_key( key_aml_value_ );
		if ( prop.valid( ) && prop.is_a< boost::int64_t >( ) )
			value = prop.value< boost::int64_t >( );
		else if ( prop.valid( ) && prop.is_a< int >( ) )
			value = boost::int64_t( prop.value< int >( ) );
		else if ( prop.valid( ) && prop.is_a< double >( ) )
			value = boost::int64_t( prop.value< double >( ) );
		else
			value = boost::int64_t( atof( pl::to_string( a->get_uri( ) ).c_str( ) ) );

		sequence_ptr seq = sequence_ptr( new sequence( stack->loops_ ) );
		stack->loops_.erase( stack->loops_.begin( ), stack->loops_.end( ) );

		for ( boost::int64_t i = 0; i < value; i ++ )
		{
			seq->start( );
			stack->push( i );
			stack->run( seq );
		}
	}
	else
	{
		stack->loops_.push_back( L"iter_range" );
	}
}

static void op_iter_popen( aml_stack *stack )
{
	if ( -- stack->loop_count_ == 0 )
	{
		boost::regex pattern( " " );
		std::string replace( "\\\\ " );

		stack->state_ = 0;

		ml::input_type_ptr input = stack->pop( );
		pl::wstring command;
		for ( int i = input->slot_count( ) - 1; i >= 0; i -- )
		{
			if ( input->fetch_slot( i )->get_uri( ).find( L" " ) != pl::wstring::npos )
			{
				std::string in = pl::to_string( input->fetch_slot( i )->get_uri( ) );
				command += pl::to_wstring( boost::regex_replace( in, pattern, replace, boost::match_default | boost::format_all ) ) + L" ";
			}
			else
				command += input->fetch_slot( i )->get_uri( ) + L" ";
		}

		FILE *pipe = stack_popen( pl::to_string( command ).c_str( ), "r" );
		char temp[ 1024 ];
	
		sequence_ptr seq = sequence_ptr( new sequence( stack->loops_ ) );
		stack->loops_.erase( stack->loops_.begin( ), stack->loops_.end( ) );

		while( fgets( temp, 1024, pipe ) )
		{
			seq->start( );
			if ( temp[ strlen( temp ) - 1 ] == '\n' ) temp[ strlen( temp ) - 1 ] = '\0';
			stack->push( ml::input_type_ptr( new input_value( pl::to_wstring( temp ) ) ) );
			stack->run( seq );
		}
	
		stack_pclose( pipe );
	}
	else
	{
		stack->loops_.push_back( L"iter_popen" );
	}
}

static void op_iter_aml( aml_stack *stack )
{
	if ( -- stack->loop_count_ == 0 )
	{
		stack->state_ = 0;

		ml::input_type_ptr input = stack->pop( );
		pl::wstring command;
		for ( int i = input->slot_count( ) - 1; i >= 0; i -- )
		{
			if ( command != L"" && input->fetch_slot( i )->get_uri( ).find( L" " ) != pl::wstring::npos )
				command += L"\"" + input->fetch_slot( i )->get_uri( ) + L"\" ";
			else
				command += input->fetch_slot( i )->get_uri( ) + L" ";
		}
	

		std::ostringstream stream;
		FILE *pipe = stack_popen( pl::to_string( command ).c_str( ), "r" );
		char temp[ 1024 ];
	
		sequence_ptr seq = sequence_ptr( new sequence( stack->loops_ ) );
		stack->loops_.erase( stack->loops_.begin( ), stack->loops_.end( ) );

		while( fgets( temp, 1024, pipe ) )
			stream << temp;

		std::istringstream file( stream.str( ) );
		stack->parse_stream( file );

		stack_pclose( pipe );
	}
	else
	{
		stack->loops_.push_back( L"iter_aml" );
	}
}

static bool key_sort( const pcos::key &k1, const pcos::key &k2 )
{
	return strcmp( k1.as_string( ), k2.as_string( ) ) < 0;
}

static std::string prop_type( const pl::pcos::property &p )
{
	if ( p.is_a< double >( ) )
		return "double";
	else if ( p.is_a< int >( ) )
		return "int";
	else if ( p.is_a< boost::int64_t >( ) )
		return "int64_t";
	else if ( p.is_a< boost::uint64_t >( ) )
		return "uint64_t";
	else if ( p.is_a< pl::wstring >( ) )
		return "wstring";
	else if ( p.is_a< std::vector< double > >( ) )
		return "vector<double>";
	else if ( p.is_a< std::vector< int > >( ) )
		return "vector<int>";
	else
		return "<unknown>";
}

static std::string prop_value( const pl::pcos::property &p )
{
	std::ostringstream stream;
	if ( p.is_a< double >( ) )
		stream << p.value< double >( );
	else if ( p.is_a< int >( ) )
		stream << p.value< int >( );
	else if ( p.is_a< boost::int64_t >( ) )
		stream << p.value< boost::int64_t >( );
	else if ( p.is_a< boost::uint64_t >( ) )
		stream << p.value< boost::uint64_t >( );
	else if ( p.is_a< pl::wstring >( ) )
		stream << "\"" << pl::to_string( p.value< pl::wstring >( ) ) << "\"";
	else if ( p.is_a< std::vector< double > >( ) )
		stream << "<unsupported>";
	else if ( p.is_a< std::vector< int > >( ) )
		stream << "<unsupported>";
	else
		stream << "<unknown>";
	return stream.str( );
}

static void op_iter_props( aml_stack *stack )
{
	if ( -- stack->loop_count_ == 0 )
	{
		stack->state_ = 0;

		ml::input_type_ptr input = stack->pop( );

		const pl::pcos::property_container &props = input->properties( );
		pl::pcos::key_vector keys = props.get_keys( );

		std::sort( keys.begin( ), keys.end( ), key_sort );

		sequence_ptr seq = sequence_ptr( new sequence( stack->loops_ ) );
		stack->loops_.erase( stack->loops_.begin( ), stack->loops_.end( ) );

		for( pl::pcos::key_vector::iterator it = keys.begin( ); it != keys.end( ); it ++ )
		{
			std::string name( ( *it ).as_string( ) );
			pl::pcos::property p = props.get_property_with_string( name.c_str( ) );
			if ( name == "debug" || name.find( ".aml_" ) == 0 ) continue;

			seq->start( );
			stack->push( ml::input_type_ptr( new input_value( pl::to_wstring( name ) ) ) );
			stack->push( ml::input_type_ptr( new input_value( pl::to_wstring( prop_type( p ) ) ) ) );
			stack->push( ml::input_type_ptr( new input_value( pl::to_wstring( prop_value( p ) ) ) ) );
			stack->run( seq );
		}

		stack->push( input );
	}
	else
	{
		stack->loops_.push_back( L"iter_props" );
	}
}

static void op_depth( aml_stack *stack )
{
	stack->push( double( stack->inputs_.size( ) ) );
}

static void op_variable( aml_stack *stack )
{
	if ( stack->next_op( op_variable ) )
	{
		ml::input_type_ptr a = stack->pop( );
		std::map< pl::wstring, ml::input_type_ptr > &locals = stack->get_locals( );
		if ( locals.find( a->get_uri( ) ) == locals.end( ) )
			locals[ a->get_uri( ) ] = ml::input_type_ptr( );
		else
			throw std::string( "Attempt to create a duplicate local var " + pl::to_string( a->get_uri( ) ) );
	}
}

static void op_variable_assign( aml_stack *stack )
{
	if ( stack->next_op( op_variable_assign ) )
	{
		ml::input_type_ptr a = stack->pop( );
		ml::input_type_ptr b = stack->pop( );
		std::map< pl::wstring, ml::input_type_ptr > &locals = stack->get_locals( );
		if ( locals.find( a->get_uri( ) ) == locals.end( ) )
			locals[ a->get_uri( ) ] = ml::input_type_ptr( );
		else
			throw std::string( "Attempt to create a duplicate local var " + pl::to_string( a->get_uri( ) ) );
		stack->assign_local( a->get_uri( ), b );
	}
}

static void op_assign( aml_stack *stack )
{
	ml::input_type_ptr a = stack->pop( );
	ml::input_type_ptr b = stack->pop( );
	stack->assign_local( a->get_uri( ), b );
}

static void op_deref( aml_stack *stack )
{
	ml::input_type_ptr a = stack->pop( );
	stack->deref_local( a->get_uri( ) );
}

static void op_decap( aml_stack *stack )
{
	ml::input_type_ptr a = stack->pop( );
	if ( a->slot_count( ) )
	{
		if ( a->property( "original_input" ).valid( ) )
		{
			stack->push( a->property( "original_input" ).value< ml::input_type_ptr >( ) );
		}
		else
		{
			for ( size_t i = 0; i < a->slot_count( ); i ++ )
				stack->push( a->fetch_slot( i ) );
		}
	}
	else
	{
		stack->push( a );
	}
}

static void op_slots( aml_stack *stack )
{
	ml::input_type_ptr a = stack->pop( );
	stack->push( a );
	stack->push( double( a->slot_count( ) ) );
}

static void op_connect( aml_stack *stack )
{
	int a = int( stack->pop_value( ) );
	ml::input_type_ptr b = stack->pop( );
	ml::input_type_ptr c = stack->pop( );
	if ( a >= 0 && size_t( a ) < c->slot_count( ) )
		c->connect( b, size_t( a ) );
	else
		throw std::string( "Invalid slot index for connect" );
	stack->push( c );
}

static void op_slot( aml_stack *stack )
{
	int a = int( stack->pop_value( ) );
	ml::input_type_ptr b = stack->pop( );
	stack->push( b );
	if ( a >=0 && size_t( a ) < b->slot_count( ) )
		stack->push( b->fetch_slot( size_t( a ) ) );
	else
		throw std::string( "Invalid slot index for fetch" );
}

static void op_recover( aml_stack *stack )
{
	ml::input_type_ptr slot = stack->parent_->fetch_slot( 0 );
	stack->push( slot );
}

static void op_fetch( aml_stack *stack )
{
	int a = int( stack->pop_value( ) );
	ml::input_type_ptr b = stack->pop( );
	b->sync( );
	stack->push( b );
	if ( a < 0 )
		a = b->get_frames( ) + a;
	ml::frame_type_ptr f = b->fetch( a );
	stack->push( ml::input_type_ptr( new frame_value( f, b ) ) );
}

static void op_define( aml_stack *stack )
{
	if ( stack->next_op( op_define ) )
	{
		ml::input_type_ptr a = stack->pop( );
		stack->print_word( a->get_uri( ) );
	}
}

static void op_log_level( aml_stack *stack )
{
	int a = int( stack->pop_value( ) );
	pl::set_log_level( a );
}

static void op_render( aml_stack *stack )
{
	ml::input_type_ptr input = stack->pop( );
	input->sync( );
	if ( input->get_frames( ) )
	{
		for ( int i = 0; i < input->get_frames( ); i ++ )
		{
			ml::frame_type_ptr frame = input->fetch( i );
			if ( frame->in_error( ) )
				break;
		}
	}
	else
	{
		*( stack->output_ ) << pl::to_string( input->get_uri( ) ) << std::endl;
	}
}

static void op_donor( aml_stack *stack )
{
	ml::input_type_ptr a = stack->pop( );
	if ( a->get_uri( ).find( L"filter:" ) == 0 )
	{
		ml::filter_type_ptr filter = ml::create_filter( a->get_uri( ).substr( 7 ) );
		for( int i = 0; i < int( filter->slot_count( ) ); i ++ )
			stack->push( i );
		stack->push( filter );
	}
	else if ( a->get_uri( ).find( L"input:" ) == 0 )
	{
		ml::input_type_ptr input = ml::create_delayed_input( a->get_uri( ).substr( 6 ) );
		stack->push( input );
	}
	else
	{
		stack->push( ml::create_input( "nothing:" ) );
	}
}

static void op_transplant( aml_stack *stack )
{
	ml::input_type_ptr a = stack->pop( );
	for( int i = 0; i < int( a->slot_count( ) ); i ++ )
		a->connect( ml::input_type_ptr( ), size_t( i ) );
	stack->push( a );
}

static void op_popen( aml_stack *stack )
{
	ml::input_type_ptr input = stack->pop( );
	pl::wstring command;
	for ( int i = input->slot_count( ) - 1; i >= 0; i -- )
	{
		if ( input->fetch_slot( i )->get_uri( ).find( L" " ) != pl::wstring::npos )
			command += L"\"" + input->fetch_slot( i )->get_uri( ) + L"\" ";
		else
			command += input->fetch_slot( i )->get_uri( ) + L" ";
	}

	FILE *pipe = stack_popen( pl::to_string( command ).c_str( ), "r" );
	char temp[ 1024 ];
	int count = 0;

	while( fgets( temp, 1024, pipe ) )
	{
		if ( temp[ strlen( temp ) - 1 ] == '\n' ) temp[ strlen( temp ) - 1 ] = '\0';
		stack->push( ml::input_type_ptr( new input_value( pl::to_wstring( temp ) ) ) );
		count ++;
	}

	stack_pclose( pipe );
	stack->push( double( count ) );
}

static void op_path( aml_stack *stack )
{
	olib::t_path path = stack->paths_.back( );
	stack->push( ml::input_type_ptr( new input_value( olib::opencorelib::str_util::to_wstring( path.string( ) ) ) ) );
}

#define const_aml_stack const_cast< input_aml_stack * >

class input_aml_stack : public ml::input_type
{
	public:
		typedef fn_observer< input_aml_stack > observer;
		
		input_aml_stack( const pl::wstring &resource )
			: input_type( )
			, prop_stdout_( pcos::key::from_string( "stdout" ) )
			, stack_( this, prop_stdout_ )
			, stream_( )
			, tos_( )
			, resource_( resource )
			, prop_command_( pcos::key::from_string( "command" ) )
			, prop_parse_( pcos::key::from_string( "parse" ) )
			, prop_commands_( pcos::key::from_string( "commands" ) )
			, prop_result_( pcos::key::from_string( "result" ) )
			, prop_redirect_( pcos::key::from_string( "redirect" ) )
			, prop_deferred_( key_deferred_ )
			, prop_threaded_( pcos::key::from_string( "threaded" ) )
			, prop_query_( pcos::key::from_string( "query" ) )
			, prop_handled_( pcos::key::from_string( "handled" ) )
			, prop_token_( pcos::key::from_string( "token" ) )
			, obs_command_( new observer( const_aml_stack( this ), &input_aml_stack::push ) )
			, obs_parse_( new observer( const_aml_stack( this ), &input_aml_stack::parse ) )
			, obs_commands_( new observer( const_aml_stack( this ), &input_aml_stack::push_commands ) )
			, obs_deferred_( new observer( const_aml_stack( this ), &input_aml_stack::deferred ) )
		{
			properties( ).append( prop_command_ = pl::wstring( L"" ) );
			properties( ).append( prop_parse_ = pl::wstring( L"" ) );
			properties( ).append( prop_commands_ = pl::wstring_list( ) );
			properties( ).append( prop_result_ = pl::wstring( L"OK" ) );
			properties( ).append( prop_redirect_ = 0 );
			properties( ).append( prop_stdout_ = std::string( "" ) );
			properties( ).append( prop_deferred_ = int( 0 ) );
			properties( ).append( prop_threaded_ = int( 0 ) );
			properties( ).append( prop_query_ = pl::wstring( L"" ) );
			prop_command_.set_always_notify( true );
			prop_parse_.set_always_notify( true );
			prop_commands_.set_always_notify( true );
			prop_query_.set_always_notify( true );
			prop_stdout_.set_always_notify( true );
			properties( ).append( prop_handled_ = int( 0 ) );
			properties( ).append( prop_token_ = pl::wstring( L"" ) );
			prop_command_.attach( obs_command_ );
			prop_parse_.attach( obs_parse_ );
			prop_commands_.attach( obs_commands_ );
			prop_deferred_.attach( obs_deferred_ );

			if ( resource.find( L"aml_stack:" ) == 0 )
				prop_command_ = resource.substr( 10 );
			else
				prop_command_ = resource;

			if ( !stack_.empty( ) )
				prop_command_ = pl::wstring( L"." );
		}

		~input_aml_stack( )
		{
			stop_threads( tos_ );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		void stop_threads( ml::input_type_ptr input )
		{
#if 0
			if ( input )
			{
				if ( input->get_uri( ) == L"threader" )
					input->property( "active" ) = 0;
				for ( size_t index = 0; index < input->slot_count( ); index ++ )
					stop_threads( input->fetch_slot( index ) );
			}
#endif
		}

		void push( )
		{
			stack_.set_output( &stream_ );

			try
			{
				stack_.push( prop_command_.value< pl::wstring >( ) );
				prop_result_ = pl::to_wstring( "OK" );
			}
			catch( std::string exc )
			{
				stack_.reset( );
				prop_result_ = pl::to_wstring( exc );
			}

			if ( stream_.str( ) != "" )
			{
				if ( prop_redirect_.value< int >( ) )
					prop_stdout_ = stream_.str( );
				else
					std::cout << stream_.str( );

				stream_.str( "" );
			}
		}

		void parse( )
		{
			stack_.set_output( &stream_ );

			try
			{
				std::istringstream stream( pl::to_string( prop_parse_.value< pl::wstring >( ) ) );
				stack_.parse_stream( stream );
				prop_result_ = pl::to_wstring( "OK" );
			}
			catch( std::string exc )
			{
				stack_.reset( );
				prop_result_ = pl::to_wstring( exc );
			}

			if ( stream_.str( ) != "" )
			{
				if ( prop_redirect_.value< int >( ) )
					prop_stdout_ = stream_.str( );
				else
					std::cout << stream_.str( );

				stream_.str( "" );
			}
		}

		void push_commands( )
		{
			if ( prop_commands_.value< pl::wstring_list >( ).size( ) != 0 )
			{
				stack_.set_output( &stream_ );

				try
				{
					stack_.run( prop_commands_.value< pl::wstring_list >( ), true );
					prop_result_ = pl::to_wstring( "OK" );
				}
				catch( std::string exc )
				{
					stack_.reset( );
					prop_result_ = pl::to_wstring( exc );
				}

				if ( stream_.str( ) != "" )
				{
					if ( prop_redirect_.value< int >( ) )
						prop_stdout_ = stream_.str( );
					else
						std::cout << stream_.str( );
	
					stream_.str( "" );
				}
				prop_commands_.set( pl::wstring_list( ) );
			}
		}

		void defer_composites( ml::input_type_ptr filter, int defer )
		{
    		if ( filter )
    		{
        		if ( filter->properties( ).get_property_with_key( key_deferred_ ).valid( ) )
            		filter->properties( ).get_property_with_key( key_deferred_ ) = defer;
        		size_t slot = filter->slot_count( );
        		while( slot -- )
            		defer_composites( filter->fetch_slot( slot ), defer );
    		}
		} 

		void deferred( )
		{
			defer_composites( tos_, prop_deferred_.value< int >( ) );
		}

		virtual const size_t slot_count( ) const 
		{ return 1; }

		virtual bool connect( ml::input_type_ptr input, size_t index = 0 ) 
		{ 
			if ( index == 0 )
				tos_ = input;
			return index == 0; 
		}

		virtual ml::input_type_ptr fetch_slot( size_t = 0 ) const 
		{ return tos_; }

		// Basic information
		virtual const pl::wstring get_uri( ) const { return resource_; }
		virtual const pl::wstring get_mime_type( ) const { return L"text/aml"; }

		// Audio/Visual
		virtual int get_frames( ) const { return tos_ ? tos_->get_frames( ) : 0; }
		virtual bool is_seekable( ) const { return tos_ ? tos_->is_seekable( ) : false; }
		virtual int get_video_streams( ) const { return tos_ ? tos_->get_video_streams( ) : 0; }
		virtual int get_audio_streams( ) const { return tos_ ? tos_->get_audio_streams( ) : 0; }

	protected:
		void do_fetch( ml::frame_type_ptr &result )
		{
			if ( tos_ )
			{
				tos_->seek( get_position( ) );
				result = tos_->fetch( );
			}
		}

	private:
		pl::pcos::property prop_stdout_;
		aml_stack stack_;
		std::ostringstream stream_;
		ml::input_type_ptr tos_;
		pl::wstring resource_;
		pl::pcos::property prop_command_;
		pl::pcos::property prop_parse_;
		pl::pcos::property prop_commands_;
		pl::pcos::property prop_result_;
		pl::pcos::property prop_redirect_;
		pl::pcos::property prop_deferred_;
		pl::pcos::property prop_threaded_;
		pl::pcos::property prop_query_;
		pl::pcos::property prop_handled_;
		pl::pcos::property prop_token_;
		boost::shared_ptr< pcos::observer > obs_command_;
		boost::shared_ptr< pcos::observer > obs_parse_;
		boost::shared_ptr< pcos::observer > obs_commands_;
		boost::shared_ptr< pcos::observer > obs_deferred_;
};

ml::input_type_ptr ML_PLUGIN_DECLSPEC create_input_aml_stack( const pl::wstring &resource )
{
	return ml::input_type_ptr( new input_aml_stack( resource ) );
}

} }

#ifdef WIN32
#pragma warning(pop)
#endif

