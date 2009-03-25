#ifndef _CORE_FACTORY_H_
#define _CORE_FACTORY_H_

#include <boost/utility.hpp>
#include <boost/function.hpp>

/**@file Factory.h	The whole file contains a reimplementation of the Loki factory
					pattern, with the only exception that the product isn't T* but
					boost::shared_ptr<T> instead. This is because just returning
					T* will make types that inherit from boost::enable_shared_from_this<T>
					fail when calling shared_from_this (since the weak_ptr keept internally
					in the class isn't initialized!). So, if you're using 
					enable_shared_from_this, use this factory implementation instead.

					The code has been enhanced with the common error handling system 
					(ARENFORCE, ARASSERT) and a function that makes it possible to 
					inspect registered types.

					@author Mats Lindel&ouml;f. */

#ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
	#pragma warning( push )
		#pragma warning(disable: 4099) //'Loki::conversion<void,void>' : type name first seen using 'struct' now seen using 'class'
#endif

#include <loki/AssocVector.h>
#include <loki/LokiTypeInfo.h>

#ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
	#pragma warning( pop )
#endif

namespace olib
{
	namespace opencorelib
	{
        /// Throws a descriptive base_exception when a factory can't find a product id.
		template <typename identifier_type, class abstract_product>
		class default_factory_error
		{
		public:

            /// Builds a string with all registered class names and throws an exception.
			static boost::shared_ptr<abstract_product> 
						on_unknown_type(	const identifier_type& in_type_id,
										const std::vector< identifier_type >& registered_ids	)
			{
				typedef typename std::vector<identifier_type>::const_iterator iter;
				iter curr(registered_ids.begin()), end(registered_ids.end());
				t_string all_registered_ids;
				for( ; curr != end; ++curr )
				{
					all_registered_ids.append( str_util::to_t_string(*curr) );
					if( curr + 1 != end ) all_registered_ids.append(_T(", "));
				}

				ARENFORCE(invoke_enforce::always_fail())(in_type_id)(all_registered_ids).msg("unknown type");
				return boost::shared_ptr<abstract_product>();
			}
		};

        /// Creates shared_ptr< abstrac_product > using registered product creators.
        /** @param abstract_product The interface that created objects must implement.
            @param identifier_type The type used to identify the correct creator.
                        A good one could be std::string. 
            @param default_factory_error A class with a static function named 
                    on_unknown_type, called when the id of a product_creator can't be found */
		template
		<
			class abstract_product, 
			class identifier_type,
			template<typename, class>
			class factory_error_policy = default_factory_error
		>
		class factory 
		:	public factory_error_policy<identifier_type, abstract_product>,
				public boost::noncopyable
		{
		public:
            /// A product_creator is a function object that creates an object that implements abstract_product.
			typedef boost::function< boost::shared_ptr<abstract_product> () > product_creator;

            /// Register a new product creator.
            /** @param id The id of the product_creator.
                @param creator The function object that creates a shared_ptr< abstract_product >
                @return true is registration succeeded, false otherwise */
			bool register_product_creator(const identifier_type& id, product_creator creator)
			{
				return associations_.insert(
					id_to_product_map::value_type(id, creator)).second;
			}

            /// Unregister a product creator.
            /** @param id The id to find and remove.
                @return true if the id was found, false otherwise. */
			bool unregister_product_creator(const identifier_type& id)
			{
				return associations_.erase(id) == 1;
			}

			/// Unregister all registered product creators.
			void clear()
			{
				associations_.clear();
			}

            /// Creates an object that implements abstract_product, using a registered product_creator.
            /** @param id The id of the product_creator to use to create the instance.
                @return An instance of an object implementing the abstract_product interface.
                @throws base_exception if the id can't be found. */
			boost::shared_ptr<abstract_product> create_object(const identifier_type& id)
			{
				typename id_to_product_map::iterator i = associations_.find(id);
				if (i != associations_.end())
				{
					return (i->second)();
				}

				std::vector< identifier_type > reg_ids;
				typename id_to_product_map::iterator it(associations_.begin()), end_it(associations_.end());
				for( ; it != end_it; ++it)
				{
					reg_ids.push_back(it->first);
				}

				return on_unknown_type(id, reg_ids );
			}

            /// Returns a map of one instance of all registred product_creators.
			void all_objects( std::map< identifier_type, boost::shared_ptr<abstract_product> >& outmap )
			{
				typename id_to_product_map::iterator it(associations_.begin()), end_it(associations_.end());
				for( ; it != end_it; ++it)
				{
					outmap[it->first] = (it->second)();
				}
			}

		private:
			typedef Loki::AssocVector<identifier_type, product_creator> id_to_product_map;
			id_to_product_map associations_;
		};

        /// Can clone objects based on their type_info. 
        /** To be able to clone instances of a certain class,
            the class must have a static clone function and that function
            must be registered with an instance of the 
            clone_factory. Using the c++ typesystem, the factory
            can find the correct clone function and do the clone.
            This class is not currently used by amf.  */
		template
		<
			class abstract_product, 
				template<typename, class>
				class factory_error_policy = default_factory_error
		>
		class clone_factory
		:	public factory_error_policy<Loki::TypeInfo, abstract_product>,
			public boost::noncopyable
		{
		public:
			typedef factory_error_policy<Loki::TypeInfo, abstract_product> base_class;
			typedef boost::function< boost::shared_ptr<abstract_product> 
				(const boost::shared_ptr<abstract_product>&) > product_creator;

			bool register_product_creator(const Loki::TypeInfo& ti, product_creator creator)
			{
				return associations_.insert(
					id_to_product_map::value_type(ti, creator)).second;
			}

			bool unregister_product_creator(const Loki::TypeInfo& id)
			{
				return associations_.erase(id) == 1;
			}

			void clear()
			{
				associations_.clear();
			}

			boost::shared_ptr<abstract_product> create_object(const boost::shared_ptr<abstract_product>& model)
			{
				if (model == 0) return 0;

				typename id_to_product_map::iterator i = 
					associations_.find(typeid(*model));
				if (i != associations_.end())
				{
					return (i->second)(model);
				}
				
				std::vector< std::string > reg_ids;
				typename id_to_product_map::iterator it(associations_.begin()), end_it(associations_.end());
				for( ; it != end_it; ++it)
				{
					reg_ids.push_back(it->first.name());
				}
				
				return base_class::on_unknown_type(typeid(*model), reg_ids);
			}

		private:
			typedef Loki::AssocVector<Loki::TypeInfo, product_creator> id_to_product_map;
			id_to_product_map associations_;
		};

	}
}

#endif //_CORE_FACTORY_H_
