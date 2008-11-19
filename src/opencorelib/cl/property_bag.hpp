#ifndef _CORE_PROPERTY_BAG_H_
#define _CORE_PROPERTY_BAG_H_

#include <boost/any.hpp>
#include "./minimal_string_defines.hpp"

namespace olib
{
   namespace opencorelib
    {
        /// Contains arbitrary key-value pairs.
        class CORE_API property_bag
        {
        public:
            typedef std::map< t_string, boost::any > bag_type;
            typedef bag_type::iterator iterator;
            typedef bag_type::const_iterator const_iterator;
            typedef bag_type::reverse_iterator reverse_iterator;
            typedef bag_type::const_reverse_iterator const_reverse_iterator;
            
            
            property_bag();

            /// Get the value for the given key
            /** @param key The key to look for in the bag.
                @return The value stored for that key.
                @throw base_exception if the key can't be found */
            const boost::any& operator[]( const t_string& key ) const;

            /// Get the value for a given key, if no value exists, create an empty.
            /** @param key The key to look for.
                @return The existing value, if present, or a new empty boost::any instance.
                        The value can be set/changed by the caller. */
            boost::any& operator[]( const t_string& key );


            /// Returns true if there is a key-value pair for the given key.
            bool get_is_key_present( const t_string& key ) const;

            /// Clear the bag of all properties.
            void clear();

            /// Is the bag empty or not.
            bool empty() const { return m_key_values.empty(); }

            /// Get the number of elements currently stored in the bag.
            size_t size() const { return m_key_values.size(); }
            
            const bag_type& get_bag() const;

        private:
            bag_type m_key_values;

        };
    }
}

#endif // _CORE_PROPERTY_BAG_H_

