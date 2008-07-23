
// oil - A oil plugin to ml.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openimagelib/il/openimagelib_plugin.hpp>
#include <openpluginlib/pl/utf8_utils.hpp>
#include <openpluginlib/pl/pcos/observer.hpp>

#ifdef WIN32
#include <windows.h>
#endif // WIN32

#include <iostream>
#include <cstdlib>
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <sstream>

#include <boost/thread/recursive_mutex.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/regex.hpp>

namespace pl = olib::openpluginlib;
namespace plugin = olib::openmedialib::ml;
namespace il = olib::openimagelib::il;

#define ML_HAS_REFRESH_CACHE

namespace olib { namespace openmedialib { namespace ml { 

// Query structure used
struct il_query_traits : public pl::default_query_traits
{
	il_query_traits( const pl::wstring& filename, const pl::wstring &type )
		: filename_( filename )
		, type_( type )
	{ }
		
	pl::wstring to_match( ) const
	{ return filename_; }

	pl::wstring libname( ) const
	{ return pl::wstring( L"openimagelib" ); }

	pl::wstring type( ) const
	{ return pl::wstring( type_ ); }
	
	const pl::wstring filename_;
	const pl::wstring type_;
};

class ML_PLUGIN_DECLSPEC oil_frame : public frame_type
{
	public:
		oil_frame( input_type *input, int position, image_type_ptr image )
		{
			set_position( position );
			set_image( image );
		}
};

class ML_PLUGIN_DECLSPEC oil_input : public input_type
{
	public:
		typedef pl::wstring::size_type size_type;

		// Constructor and destructor
		oil_input( const pl::wstring &resource ) 
			: input_type( )
			, resource_( resource )
			, path_( "" )
			, files_( )
			, width_( 0 )
			, height_( 0 )
			, prop_sar_num_( pcos::key::from_string( "sar_num" ) )
			, prop_sar_den_( pcos::key::from_string( "sar_den" ) )
			, prop_fps_num_( pcos::key::from_string( "fps_num" ) )
			, prop_fps_den_( pcos::key::from_string( "fps_den" ) )
			, last_image_name_( "" )
			, prop_cache_( pcos::key::from_string( "cache" ) )
			, prop_regexp_( pcos::key::from_string( "regexp" ) )
		{ 
			properties( ).append( prop_sar_num_ = 1 );
			properties( ).append( prop_sar_den_ = 1 );
			properties( ).append( prop_fps_num_ = 25 );
			properties( ).append( prop_fps_den_ = 1 );
			properties( ).append( prop_cache_ = 0 );
			properties( ).append( prop_regexp_ = 0 );
		}

		virtual ~oil_input( ) 
		{ 
		}

		inline bool matches( const pl::string &file, const pl::string &wild )
		{
			size_type f = 0;
			size_type w = 0;

			while( f < file.size( ) && w < wild.size( ) )
			{
				if ( wild[ w ] == '*' )
				{
					w ++;
					if ( w == wild.size( ) )
						f = file.size( );
					while ( f != file.size( ) && file[ f ] != wild[ w ] )
						f ++;
				}
				else if ( wild[ w ] == '?' || file[ f ] == wild[ w ] )
				{
					f ++;
					w ++;
				}
				else
				{
					return false;
				}
			}

			return file.size( ) == f && wild.size( ) == w;
		}

		void find_matches( const pl::string &pattern )
		{
			typedef pl::discovery<il_query_traits> discovery;

			size_type i = pattern.rfind( "/" );

			// Pattern to path and wildcard
			path_ = i == std::string::npos ? "." : pl::string( pattern, 0, i );
			pl::string wild = i == pl::string::npos ? pl::string( pattern ) : pl::string( pattern, i + 1 );

			boost::regex re;
			if ( prop_regexp_.value< int >( ) )
			{
				path_ = i == std::string::npos ? "." : pl::string( pattern, 0, i );
				wild = i == pl::string::npos ? pl::string( pattern ) : pl::string( pattern, i + 1 );
				re = boost::regex( wild.c_str( ) );
			}

			// Obtain the path object
			boost::filesystem::path path( path_.c_str( ), boost::filesystem::native );

			// Iterate through the contents of the directory and find matches
			boost::filesystem::directory_iterator end_itr;
			for ( boost::filesystem::directory_iterator itr( path ); itr != end_itr; ++itr )
			{
				if ( prop_regexp_.value< int >( ) && boost::regex_match( itr->leaf( ).c_str( ), re ) )
					files_.push_back( itr->leaf( ).c_str( ) );
				else if ( !prop_regexp_.value< int >( ) && matches( itr->leaf( ).c_str( ), wild ) )
					files_.push_back( itr->leaf( ).c_str( ) );
			}

			// Sanity check
			if ( files_.size( ) )
			{
				// Ensure sort order on file name
				sort( files_.begin( ), files_.end( ) );

				// Obtain the OIL plugin object
				pl::string full = path_ + "/" + files_[ 0 ];
				il_query_traits query( pl::to_wstring( full ), L"input" );
				discovery plugins( query );
				if ( plugins.size( ) != 0 )
				{
					discovery::const_iterator it = plugins.begin( );
					plug_ = boost::shared_dynamic_cast<il::openimagelib_plugin>( it->create_plugin( "" ) );
					if ( plug_ == 0 ) 
						files_.clear( );
				}
				else
				{
					files_.clear( );
				}
			}
		}

		void process_sequence( const pl::string &sequence )
		{
			pl::string copy = sequence;

			// Horribly clunky parsing of additional sequence arguments
			while ( copy != "" )
			{
				pl::string property = pl::string( copy, 0, copy.find( ":" ) );
				pl::string token = pl::string( property, 0, property.find( "=" ) );
				pl::string value = property.find( "=" ) != pl::string::npos ? pl::string( property, property.find( "=" ) + 1 ) : "";

				if ( token == "fps" && value.find( ',' ) != pl::string::npos )
				{
					prop_fps_num_ = atoi( value.c_str( ) );
					prop_fps_den_ = atoi( pl::string( value, value.find( ',' ) + 1 ).c_str( ) );
				}
				else if ( token == "sar" && value.find( ',' ) != pl::string::npos )
				{
					prop_sar_num_ = atoi( value.c_str( ) );
					prop_sar_den_ = atoi( pl::string( value, value.find( ',' ) + 1 ).c_str( ) );
				}
				else if ( token == "cache" )
				{
					prop_cache_ = atoi( value.c_str( ) );
				}
				else if ( token == "regexp" )
				{
					prop_regexp_ = atoi( value.c_str( ) );
				}
				else
				{
					std::cerr << "OML OIL Input Plugin: ignoring token " << token << " = " << value << std::endl;
				}

				if ( copy == property ) break;
				copy = pl::string( copy, copy.find( ":" ) + 1 );
			}
		}

		// Basic information
		virtual const pl::wstring get_uri( ) const { return resource_; }
		virtual const pl::wstring get_mime_type( ) const { return L""; }
		virtual bool has_video( ) const { return files_.size( ) > 0; }
		virtual bool has_audio( ) const { return false; }

		// Audio/Visual
		virtual int get_frames( ) const { return files_.size( ); }
		virtual bool is_seekable( ) const { return true; }

		// Visual
		virtual void get_fps( int &num, int &den ) const { num = prop_fps_num_.value< int >( ); den = prop_fps_den_.value< int >( ); }
		virtual void get_sar( int &num, int &den ) const { num = prop_sar_num_.value< int >( ); den = prop_sar_den_.value< int >( ); }
		virtual int get_video_streams( ) const { return 1; }
		virtual int get_width( ) const { return width_; }
		virtual int get_height( ) const { return height_; }

		// Audio
		virtual int get_audio_streams( ) const { return 0; }

		virtual double fps( ) const
		{
			int num, den;
			get_fps( num, den );
			return den != 0 ? double( num ) / double( den ) : 1;
		}

	protected:
		// Fetch method
		void do_fetch( frame_type_ptr &result )
		{
			if ( get_frames( ) > 0 )
			{
				image_type_ptr image = get_image( );
				if ( image != 0 )
				{
					width_ = image->width( );
					height_ = image->width( );
				}
				result = frame_type_ptr( new oil_frame( this, get_position( ), image ) );
			}
			else
			{
				result = frame_type_ptr( new frame_type( ) );
			}

			int num, den;
			get_sar( num, den );
			result->set_sar( num, den );
			get_fps( num, den );
			result->set_fps( num, den );
			result->set_pts( get_position( ) * 1.0 / fps( ) );
			result->set_duration( 1.0 / fps( ) );
		}

		image_type_ptr get_image( )
		{
			namespace fs = boost::filesystem;

			int index = get_position( );

			// Just make sure we're in bounds
			if ( index < 0 ) 
				index = 0;
			else if ( index >= get_frames( ) ) 
				index = get_frames( ) - 1;

			// And that the input agrees
			seek( index );

			// Get the full path to the image
			pl::string full = path_ + pl::string( "/" ) + files_[ index ];

			// To cache or not to cache
			if ( prop_cache_.value< int >( ) == 0 )
			{
				if ( full != last_image_name_ )
				{
					last_image_name_ = full;
					last_image_ = plug_->load( fs::path( full.c_str( ), fs::native ) );
                    if ( last_image_ )
                        last_image_->set_writable( false );
				}
				return last_image_;
			}
			else
			{
				if ( bucket_[ index ] == image_type_ptr( ) )
				{
					image_type_ptr image = plug_->load( fs::path( full.c_str( ), fs::native ) );
                    if ( image ) // The image may be invalid!
                    {
                        bucket_[ index ] = image;
                        bucket_[ index ]->set_writable( false );
                    }
				}
				return bucket_[ index ];
			}
		}

		void refresh_cache( frame_type_ptr frame )
		{
			if ( prop_cache_.value< int >( ) )
			{
				int index = frame->get_position( );
				image_type_ptr image = frame->get_image( );
                if ( image ) {
                    bucket_[ index ] = image;
                    bucket_[ index ]->set_writable( false );
                }
			}
		}

		virtual bool initialize( )
		{
			const pl::string resource = pl::to_string( resource_ );
			const pl::string identifier = "/sequence:";

			// Locate the sequence identifier
			size_type i = resource.rfind( identifier );

			// If we have a sequence identifier, then continue
			if ( i != std::string::npos )
			{
				// Get the pattern and args as two seperate strings
				pl::string pattern = pl::string( resource, 0, i );
				pl::string sequence = pl::string( resource, i + identifier.size( ) );
				process_sequence( sequence );
				find_matches( pattern );

				// Set up room for the cache if requested
				if ( prop_cache_.value< int >( ) && files_.size( ) <= prop_cache_.value< int >( ) )
				{
					bucket_.resize( files_.size( ) );
					for ( size_t i = 0; i < files_.size( ); i ++ )
						bucket_[ i ] = image_type_ptr( );
				}
				else
				{
					prop_cache_ = 0;
				}
			}

			return files_.size( ) > 0;
		}


	private:
		pl::wstring resource_;
		pl::string path_;
		std::vector < pl::string > files_;
		int width_;
		int height_;
		pcos::property prop_sar_num_;
		pcos::property prop_sar_den_;
		pcos::property prop_fps_num_;
		pcos::property prop_fps_den_;
		boost::shared_ptr < il::openimagelib_plugin > plug_;
		image_type_ptr last_image_;
		pl::string last_image_name_;
		pcos::property prop_cache_;
		pcos::property prop_regexp_;
		std::vector < image_type_ptr > bucket_;
};

class ML_PLUGIN_DECLSPEC oil_store : public store_type
{
	public:
		typedef std::wstring::size_type size_type;

		oil_store( const pl::wstring &resource )
			: store_type( )
			, resource_( resource )
			, path_( "" )
			, extension_( "" )
			, digits_( 5 )
			, current_( 0 )
		{
			parse_resource( pl::to_string( resource ) );
		}

		virtual ~oil_store( )
		{
		}

		void parse_resource( const pl::string &resource )
		{
			// Goncalo: Note sure if this : will be acceptable on windows - change to an @ or !
			// if you need to (and make corresponding change in regexp in the opl file)
			const pl::string identifier = "/sequence:";

			// Locate the sequence identifier
			size_type i = resource.rfind( identifier );

			// If we have a sequence identifier, then continue
			if ( i != std::string::npos )
			{
				// Get the pattern and args as two seperate strings
				pl::string pattern = pl::string( resource, 0, i );
				pl::string sequence = pl::string( resource, i + identifier.size( ) );
				parse_pattern( pattern );
				parse_sequence( sequence );
			}
		}

		void parse_pattern( const pl::string &pattern )
		{
			size_type i = pattern.rfind( "." );
			if ( i != pl::string::npos )
			{
				path_ = pl::string( pattern, 0, i );
				extension_ = pl::string( pattern, i );
			}
		}

		void parse_sequence( const pl::string &sequence )
		{
			pl::string copy = sequence;

			// Horribly clunky parsing of additional sequence arguments
			while ( copy != "" )
			{
				pl::string property = pl::string( copy, 0, copy.find( ":" ) );
				pl::string token = pl::string( property, 0, property.find( "=" ) );
				pl::string value = property.find( "=" ) != pl::string::npos ? pl::string( property, property.find( "=" ) + 1 ) : "";

				if ( token == "digits" )
				{
					digits_ = atoi( value.c_str( ) );
				}
				else
				{
					std::cerr << "OML OIL Store Plugin: ignoring token " << token << " = " << value << std::endl;
				}

				if ( copy == property ) break;
				copy = pl::string( copy, copy.find( ":" ) + 1 );
			}
}

		virtual bool push( frame_type_ptr frame )
		{
			typedef pl::discovery<il_query_traits> discovery;
			il::image_type_ptr image = frame->get_image( );
			if ( image == 0 ) return false;
			image = il::convert( image, L"r8g8b8" );

			std::ostringstream out;
			out << path_;
			out.fill( '0' );
			std::streamsize orig = out.width( digits_ );
			out << std::right << ++ current_;
			out.width( orig );
			out << extension_;

			il_query_traits query( pl::to_wstring( out.str( ).c_str( ) ), L"output" );
			discovery plugins( query );
			if ( plugins.size( ) == 0 ) return false;
			discovery::const_iterator i = plugins.begin( );
			boost::shared_ptr<il::openimagelib_plugin>plug = boost::shared_dynamic_cast<il::openimagelib_plugin>( i->create_plugin( "" ) );
			if ( plug == 0 ) return false;
			return plug->store( out.str( ), image );
		}

	private:
		pl::wstring resource_;
		pl::string path_;
		pl::string extension_;
		int digits_;
		int current_;
};

//
// Plugin object
//

class ML_PLUGIN_DECLSPEC oil_plugin : public openmedialib_plugin
{
public:
	virtual input_type_ptr input(  const pl::wstring &resource )
	{
		return input_type_ptr( new oil_input( resource ) );
	}

	virtual store_type_ptr store( const pl::wstring &resource, const frame_type_ptr & )
	{
		return store_type_ptr( new oil_store( resource ) );
	}
};

} } }

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
		*plug = new plugin::oil_plugin;
		return true;
	}
	
	ML_PLUGIN_DECLSPEC void openplugin_destroy_plugin( pl::openplugin* plug )
	{ 
		delete static_cast< plugin::oil_plugin * >( plug ); 
	}
}

