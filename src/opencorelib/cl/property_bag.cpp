#include "precompiled_headers.hpp"
#include "./property_bag.hpp"
#include "./enforce_defines.hpp"
#include "./enforce.hpp"

namespace olib
{
   namespace opencorelib
    {
        property_bag::property_bag()
        {

        }

        const boost::any& property_bag::operator[]( const t_string& key ) const
        {
            bag_type::const_iterator cit = m_key_values.find(key);
            ARENFORCE_MSG( cit != m_key_values.end(), "Key %s not found")(key);
            return cit->second;
        }

        boost::any& property_bag::operator[]( const t_string& key )
        {
            return m_key_values[key];
        }
        
        const boost::any& property_bag::get_value_for_key( const t_string& key ) const
        {
            return this->operator[](key);
        }
        
        void property_bag::set_value_for_key( const t_string& key, const boost::any& val )
        {
            m_key_values[key] = val;
        }

        bool property_bag::get_is_key_present( const t_string& key ) const
        {
            bag_type::const_iterator cit = m_key_values.find(key);
            return cit != m_key_values.end();
        }

        void property_bag::clear()
        {
            m_key_values.clear();
        }
        
        const property_bag::bag_type& property_bag::get_bag() const
        {
            return m_key_values;
        }

    }
}
