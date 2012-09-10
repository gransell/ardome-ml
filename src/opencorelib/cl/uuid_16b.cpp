#include "precompiled_headers.hpp"
#include "./uuid_16b.hpp"
#include "./enforce_defines.hpp"
#include "./enforce.hpp"

#ifndef OLIB_ON_WINDOWS
    #include <uuid/uuid.h>
#endif

namespace olib
{
   namespace opencorelib
    {
        // Examples of what to match:
        // a42790e0-7810-11cf-8f52-0040333594a3 
        // 1A57AA22-DCA5-48CE-81AC-651986ADF224
        t_regex uuid_16b::m_uuid_regex( _CT("([A-Fa-f0-9]{8})-([A-Fa-f0-9]{4})") 
                                        _CT("-([A-Fa-f0-9]{4})-([A-Fa-f0-9]{4})-([A-Fa-f0-9]{8})([A-Fa-f0-9]{4})"));

        uuid_16b::uuid_16b()
        {
            fill_with_unique_data(); 
        }

        uuid_16b::uuid_16b( bool no_init )
        {
            if(!no_init) fill_with_unique_data();
        }

        namespace 
        {
            void long_to_char_array( boost::uint8_t* d, boost::uint32_t val)
            {
                d[0] = ((boost::uint8_t*)&val)[3];
                d[1] = ((boost::uint8_t*)&val)[2];
                d[2] = ((boost::uint8_t*)&val)[1];
                d[3] = ((boost::uint8_t*)&val)[0];
            }

            void short_to_char_array( boost::uint8_t* d, boost::uint16_t val)
            {
                d[0] = ((boost::uint8_t*)&val)[1];
                d[1] = ((boost::uint8_t*)&val)[0];
            }

            boost::uint32_t hex_to_long( const t_string& str )
            {
                boost::uint32_t res(0);
                static boost::uint32_t dec_zero = static_cast<boost::int32_t>(_CT('0'));
                static boost::uint32_t hex_zero = static_cast<boost::int32_t>(_CT('A'));

                for( size_t i = 0; i < str.size(); ++i )
                {
                    boost::uint32_t c = static_cast<boost::int32_t>(std::toupper(str[i]));
                    if( std::isdigit(c)) res += c - dec_zero;
                    else res += c - hex_zero + 10; 
                    if( i < str.size() - 1 ) 
                    {
                        res <<= 4;
                    }
                }
                return res;
            }

            boost::uint16_t hex_to_short( const t_string& str )
            {
                return static_cast<boost::uint16_t>(hex_to_long(str));
            }


            /// Converts from a byte value to its two-character, uppercase
            /// hexadecimal representation.
            /// Note that this function does not write a null terminator
            /// character to its output buffer.
            /** @param val The byte value to convert.
                @param buf A pointer to the result buffer, which must contain
                       space for 2 wchar_t values.
            */
            inline void byte_val_to_hex( boost::uint8_t val, wchar_t *buf )
            {
                const wchar_t *hex_chars = _CT("0123456789ABCDEF");

                buf[ 0 ] = hex_chars[ val >> 4 ];
                buf[ 1 ] = hex_chars[ val & 0x0F ];
            }
        }

        void uuid_16b::fill_with_unique_data()
        {
            #ifdef OLIB_ON_WINDOWS
                GUID tmp;
                ARENFORCE_MSG( ::CoCreateGuid(&tmp) == S_OK , "Could not create uuid_16b instance");
                long_to_char_array( m_data, tmp.Data1 );
                short_to_char_array(m_data + 4, tmp.Data2 );
                short_to_char_array(m_data + 6, tmp.Data3 );
                for( boost::int32_t i = 0; i < 8; ++i) 
                    m_data[i + 8] = tmp.Data4[i];
            #else
                uuid_generate(m_data);
            #endif
        }

        uuid_16b::uuid_16b( const uuid_16b& other )
        {
            memcpy( m_data, other.m_data, 16);
        }

        uuid_16b& uuid_16b::operator=( const uuid_16b& other )
        {
            memcpy( m_data, other.m_data, 16);
            return *this;
        }

        /// Convert the unique identifier to a hex string.
        t_string uuid_16b::to_hex_string() const
        {
            t_string result( 36, '-' );
            wchar_t *buf = &result[0];

            for( int i = 0; i < 4; ++i, buf += 2 )
                byte_val_to_hex( m_data[ i ], buf );
            ++buf;
            for( int i = 4; i < 6; ++i, buf += 2 )
                byte_val_to_hex( m_data[ i ], buf );
            ++buf;
            for( int i = 6; i < 8; ++i, buf += 2 )
                byte_val_to_hex( m_data[ i ], buf );
            ++buf;
            for( int i = 8; i < 10; ++i, buf += 2 )
                byte_val_to_hex( m_data[ i ], buf );
            ++buf;
            for( int i = 10; i < 16; ++i, buf += 2 )
                byte_val_to_hex( m_data[ i ], buf );

            return result;
        }

        /// Create a unique id from a serialized hex string.
        uuid_16b from_hex_string( const t_string& hex_str )
        {
            uuid_16b res(true); // no init
            t_smatch match;
            
            ARENFORCE_MSG( boost::regex_match(hex_str, match, uuid_16b::m_uuid_regex ), 
                            "The passed string does not match XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXX, "
                            "where X denotes a hexadecimal digit (0-F)")(hex_str);

            long_to_char_array( res.m_data, hex_to_long(match[1].str()));
            short_to_char_array( res.m_data + 4, hex_to_short(match[2].str()));
            short_to_char_array( res.m_data + 6, hex_to_short(match[3].str()));
            short_to_char_array( res.m_data + 8, hex_to_short(match[4].str()));
            long_to_char_array( res.m_data + 10, hex_to_long(match[5].str()));
            short_to_char_array( res.m_data + 14, hex_to_short(match[6].str()));

            return res;
        }

        /// Compares for equality
        bool operator==(const uuid_16b& lhs, const uuid_16b& rhs)
        {
            return memcmp(lhs.m_data, rhs.m_data, 16) == 0;
        }

        /// Is lhs less than rhs?
        bool operator<(const uuid_16b& lhs, const uuid_16b& rhs)
        {
            return memcmp(lhs.m_data, rhs.m_data, 16) < 0;
        }



        t_ostream& operator << ( t_ostream& os, const uuid_16b& uid)
        {
            os << uid.to_hex_string();
            return os;
        }

        t_istream& operator >> ( t_istream& is, uuid_16b& gid)
        {
            t_string str;
            is >> str;
            gid = from_hex_string(str);
            return is;
        }


    }
}
