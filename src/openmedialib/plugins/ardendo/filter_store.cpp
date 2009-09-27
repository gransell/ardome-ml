// Store filter
//
// Copyright (C) 2009 Ardendo

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include "utility.hpp"

namespace aml { namespace openmedialib {

class ML_PLUGIN_DECLSPEC filter_store : public ml::filter_type
{
	public:
		filter_store( )
			: ml::filter_type( )
			, prop_enable_( pcos::key::from_string( "enable" ) )
			, prop_store_( pcos::key::from_string( "store" ) )
		{
			properties( ).append( prop_enable_ = 1 );
			properties( ).append( prop_store_ = pl::wstring( L"" ) );
		}

		virtual ~filter_store( )
		{
			if ( first_ )
				store_->push( first_->shallow( ) );
			if ( store_ )
				store_->complete( );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return true; }

		virtual const pl::wstring get_uri( ) const { return L"store"; }

	protected:
		void do_fetch( ml::frame_type_ptr &result )
		{
			acquire_values( );
			result = fetch_from_slot( );

			if ( prop_enable_.value< int >( ) )
			{
				if ( store_ == 0 && prop_store_.value< pl::wstring >( ) != L"" )
				{
					first_ = result;

					store_ = ml::create_store( prop_store_.value< pl::wstring >( ), result );

					if ( store_ )
						pass_properties( store_ );

					if ( !store_ || !store_->init( ) )
						store_ = ml::store_type_ptr( );
					else
						return;
				}

				if ( store_ )
				{
					if ( first_ && first_->get_position( ) != result->get_position( ) )
						store_->push( first_->shallow( ) );

					first_ = ml::frame_type_ptr( );

					ARENFORCE_MSG(store_->push( result->shallow( ) ),
						"Pushing to store failed. A possible cause could be insufficient disk space.");
				}
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
					{
						if ( p.is_a< int >( ) && v.is_a< int >( ) )
							p = v.value< int >( );
						else if ( p.is_a< double >( ) && v.is_a< double >( ) )
							p = v.value< double >( );
                        else if ( p.is_a< boost::uint64_t >( ) && v.is_a< boost::uint64_t >( ) )
                            p = v.value< boost::uint64_t >( );
                        else if ( p.is_a< boost::int64_t >( ) && v.is_a< boost::int64_t >( ) )
                            p = v.value< boost::int64_t >( ) ;
						else if ( v.is_a< pl::wstring >( ) )
							p.set_from_string( v.value< pl::wstring >( ) );
						else
							std::cerr << "Don't know how to match property " << name << std::endl;
					}
					else
						std::cerr << "Unknown property " << name << std::endl;
				}
			}
		}

	private:
		pcos::property prop_enable_;
		pcos::property prop_store_;

		ml::store_type_ptr store_;
		ml::frame_type_ptr first_;
};

ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_store( const pl::wstring &resource )
{
	return ml::filter_type_ptr( new filter_store( ) );
}

} }
