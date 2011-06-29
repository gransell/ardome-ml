
#include <openpluginlib/pl/log.hpp>
#include "input.hpp"
#include "frame.hpp"
#include "filter.hpp"

namespace opl  = olib::openpluginlib;
namespace pcos = olib::openpluginlib::pcos;

namespace olib { namespace openmedialib { namespace ml {

void filter_key::refresh( pcos::property input )
{
	if ( prev_value_ != input.value< opl::wstring >( ) )
	{
		prev_value_ = input.value< opl::wstring >( );

		// Obtain the value as a string
		std::string value = opl::to_string( prev_value_ ).c_str( );

		// Update the previous value
		// Check if there's a default value specified (syntax being <property>:<default>)
		std::string::size_type default_provided = value.find( ":" );

		// Determine if a default value is provided
		if ( default_provided != std::string::npos )
		{
			default_ = value.substr( default_provided + 1 );
			value = value.substr( 0, default_provided );
		}

		// Determine source based on @ rules
		if ( value.substr( 0, 2 ) == "@@" )
		{
			src_location_ = from_frame;
			src_key_ = pcos::key::from_string( value.substr( 2 ).c_str( ) );
		}
		else if ( value[ 0 ] == '@' )
		{
			src_location_ = from_filter;
			src_key_ = pcos::key::from_string( value.substr( 1 ).c_str( ) );
		}
		else
		{
			src_location_ = from_default;
			default_ = value;
		}
	}
}

void filter_key::assign( ml::frame_type_ptr &frame, filter_type *filter )
{
	// Obtain the src and dst properties
	pcos::property src( pcos::property::NULL_PROPERTY );
	pcos::property dst( pcos::property::NULL_PROPERTY );

	switch( dst_location_ )
	{
		case from_default:
			break;

		case from_filter:
			dst = filter->properties( ).get_property_with_key( dst_key_ );
			break;

		case from_frame:
			dst = frame->properties( ).get_property_with_key( dst_key_ );
			break;
	}

	switch( src_location_ )
	{
		case from_default:
			break;

		case from_filter:
			src = filter->properties( ).get_property_with_key( src_key_ );
			break;

		case from_frame:
			src = frame->properties( ).get_property_with_key( src_key_ );
			break;
	}

	// Now assign as required
	if ( dst.valid( ) )
	{
		if ( src.valid( ) && dst.is_a< int >( ) && src.is_a< double >( ) )
			dst = int( src.value< double >( ) );
		else if ( src.valid( ) )
			dst.set_from_property( src );
		else 
			dst.set_from_string( opl::to_wstring( default_ ) );
	}
}

filter_type::filter_type( )
	: input_type( )
	, slots_( )
	, keys_( )
	, position_( 0 ) 
{
	slots_.push_back( input_type_ptr( ) ); 
}

filter_type::~filter_type( )
{
}

const size_t filter_type::slot_count( ) const 
{ 
	return 1; 
}

const openpluginlib::wstring filter_type::get_mime_type( ) const
{
	return slots_[ 0 ] ? slots_[ 0 ]->get_mime_type( ) : L""; 
}

int filter_type::get_position( ) const 
{
	return position_; 
}

int filter_type::get_frames( ) const
{
	return slots_[ 0 ] ? slots_[ 0 ]->get_frames( ) : 0; 
}

bool filter_type::is_seekable( ) const 
{ 
	return slots_[ 0 ] ? slots_[ 0 ]->is_seekable( ) : false; 
}

int filter_type::get_video_streams( ) const
{ 
	return slots_[ 0 ] ? slots_[ 0 ]->get_video_streams( ) : 0; 
}

int filter_type::get_audio_streams( ) const
{
	return slots_[ 0 ] ? slots_[ 0 ]->get_audio_streams( ) : 0; 
}

int filter_type::get_audio_channels_in_stream( int stream_index ) const
{
	return slots_[ 0 ] ? slots_[ 0 ]->get_audio_channels_in_stream( stream_index ) : 0; 
}

void filter_type::on_slot_change( input_type_ptr, int ) 
{
}

input_type_ptr filter_type::fetch_slot( size_t slot ) const
{
	return slot < slots_.size( ) ? slots_[ slot ] : input_type_ptr( ); 
}

frame_type_ptr filter_type::fetch( )
{
	collated_.erase( collated_.begin( ), collated_.end( ) );
	ml::frame_type_ptr result = input_type::fetch( );
	for ( exception_list::iterator i = collated_.begin( ); i != collated_.end( ); i ++ )
		result->push_exception( ( *i ).first, ( *i ).second );
	return result;
}

bool filter_type::connect( input_type_ptr input, size_t slot )
{
	if ( slots_.size( ) != slot_count( ) )
	{
		size_t i;
		for ( i = slots_.size( ); i > slot_count( ); i -- )
			slots_.pop_back( );
		for ( i = slots_.size( ); i < slot_count( ); i ++ )
			slots_.push_back( input_type_ptr( ) );
	}

	bool result = slot < slot_count( );
	if ( result )
	{
		slots_[ slot ] = input;
		on_slot_change( input, static_cast<int>(slot) );
	}

	return result;
}

// Provides a hint to the input implementation
void filter_type::set_process_flags( int flags ) 
{
	input_type::set_process_flags( flags );
	for( std::vector< input_type_ptr >::iterator iter = slots_.begin( ); iter < slots_.end( ); iter ++ )
		if ( *iter )
			( *iter )->set_process_flags( get_process_flags( ) );
}

void filter_type::seek( const int position, const bool relative )
{
	if ( relative )
		position_ = ( position_ + position );
	else
		position_ = position;

	if ( position_ < 0 ) 
		position_ = 0;
}

void filter_type::acquire_values( ) 
{
}

bool filter_type::is_thread_safe( )
{
	size_t count = 0;
	for( std::vector< input_type_ptr >::const_iterator iter = slots_.begin( ); iter != slots_.end( ); iter ++ )
		if ( !( *iter ) || ( *iter && ( *iter )->is_thread_safe( ) ) )
			count ++;
	return count == slots_.size( );
}

frame_type_ptr filter_type::fetch_from_slot( int index, bool assign )
{
	frame_type_ptr result;
	input_type_ptr input = fetch_slot( index );
	if ( input )
	{
		input->seek( get_position( ) );
		result = input->fetch( );
		if ( result && assign )
			assign_frame_props( result );
		else if ( !result )
			PL_LOG( opl::level::error, boost::format( "Unable to retrieve frame %d from %s connected to %s" ) % 
					get_position( ) % opl::to_string( get_uri( ) ) % opl::to_string( input->get_uri( ) ) );
		if ( result )
		{
			exception_list list = result->exceptions( );
			for ( exception_list::iterator i = list.begin( ); i != list.end( ); i ++ )
				collated_.push_back( *i );
			result->clear_exceptions( );
		}
	}
	return result;
}

void filter_type::assign_frame_props( frame_type_ptr frame )
{
	if ( frame )
	{
		// Obtain the keys on the filter
		pcos::key_vector props = properties( ).get_keys( );

		// For each key...
		for( pcos::key_vector::iterator it = props.begin( ); it != props.end( ); it ++ )
		{
			// Fetch the name and value property
			std::string name( ( *it ).as_string( ) );

			if ( name[ 0 ] == '@' )
			{
				// Create the filter key if we haven't done so before
				if ( keys_.find( ( *it ).id( ) ) == keys_.end( ) )
				{
					// We need to determine where the value is obtained (coming from the @ and @@ rules)
					location loc = from_frame;

					// Strip off unique id (used to allow multiple regions in lerp and bezier filters)
					if ( name.find( ":" ) != std::string::npos )
						name = name.substr( 0, name.find( ":" ) );

					// Determine where the value is obtained and strip off identifiers
					if ( name[ 1 ] == '@' )
					{
						loc = from_frame;
						name = name.substr( 2 );
					}
					else 
					{
						loc = from_filter;
						name = name.substr( 1 );
					}

					// Construct a new filter key
					keys_[ ( *it ).id( ) ] = filter_key_ptr( new filter_key( name, loc ) );
				}

				// Obtain the filter key
				filter_key_ptr filter_key = keys_[ ( *it ).id( ) ];

				// Refresh the value from the current property
				filter_key->refresh( properties( ).get_property_with_key( *it ) );

				// Assign property
				filter_key->assign( frame, this );
			}
		}
	}
}

} } }
