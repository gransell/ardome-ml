// Store filter
//
// Copyright (C) 2009 Ardendo
// Released under the terms of the LGPL.
//
// #filter:store
// 
// Allows stores to be introduced into graphs as filters.

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"

namespace aml { namespace openmedialib {

class ML_PLUGIN_DECLSPEC filter_store : public ml::filter_simple
{
	public:
		filter_store( )
			: ml::filter_simple( )
			, prop_enable_( pcos::key::from_string( "enable" ) )
			, prop_store_( pcos::key::from_string( "store" ) )
		{
			properties( ).append( prop_enable_ = 1 );
			properties( ).append( prop_store_ = pl::wstring( L"" ) );
		}

		virtual ~filter_store( )
		{
			if ( store_ )
				store_->complete( );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return true; }

		virtual const pl::wstring get_uri( ) const { return L"store"; }

	protected:
		void do_fetch( ml::frame_type_ptr &result )
		{
			if ( !last_frame_ || last_frame_->get_position( ) != get_position( ) )
			{
				result = fetch_from_slot( );

				if ( prop_enable_.value< int >( ) )
				{
					if ( store_ == 0 )
					{
						store_ = ml::create_store( prop_store_.value< pl::wstring >( ), result );
						ARENFORCE_MSG( store_, "Failed to create a store from %s" )( prop_store_.value< pl::wstring >( ) );
						pass_properties( store_ );
						ARENFORCE_MSG( store_->init( ), "Failed to initalise a store from %s" )( prop_store_.value< pl::wstring >( ) );
					}

					ARENFORCE_MSG(store_->push( result->shallow( ) ), "Pushing to store %1% failed.") (prop_store_.value<pl::wstring>());
				}
				last_frame_ = result;
			}
			else
			{
				result = last_frame_;
			}
		}

		void pass_properties( ml::store_type_ptr &store )
		{
			std::string prefix = "@store.";
			int ps = static_cast< int >( prefix.size( ) );
			pcos::key_vector keys = properties( ).get_keys( );
			for( pcos::key_vector::iterator it = keys.begin( ); it != keys.end( ); it ++ )
			{
				std::string name( ( *it ).as_string( ) );
				if ( name.find( prefix ) == 0 )
				{
					std::string prop = name.substr( ps );
					pcos::property p = store->properties( ).get_property_with_string( prop.c_str( ) );
					pcos::property v = properties( ).get_property_with_string( name.c_str( ) );
					if ( p.valid( ) && v.valid( ) )
						p.set_from_string( v.value<pl::wstring>() );
					else
						std::cerr << "Unknown property " << name << std::endl;
				}
			}
		}

	private:
		pcos::property prop_enable_;
		pcos::property prop_store_;
		ml::frame_type_ptr last_frame_;
		ml::store_type_ptr store_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_store( const pl::wstring &resource )
{
	return ml::filter_type_ptr( new filter_store( ) );
}

} }
