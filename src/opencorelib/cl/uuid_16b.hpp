#ifndef _CORE_UUID_16B_H_
#define _CORE_UUID_16B_H_

#include "./string_defines.hpp"
#include "./typedefs.hpp"

namespace olib
{
   namespace opencorelib
    {
        /// This class encapsulates 16 bytes of globally unique data
        /** Each new instance created of this class creates a 
            new globally unique value. This can for instance be used to create
            unique id strings, by calling to_hex_string.
            @author Mats Lindel&ouml;f */
        class CORE_API uuid_16b :
            public boost::equality_comparable< uuid_16b >,
            public boost::less_than_comparable< uuid_16b >

        {
        public:
            /// Create a new unique identifier.
            uuid_16b();

            /// Copy constructor
            uuid_16b( const uuid_16b& other );

            /// Assignment operator
            uuid_16b& operator=( const uuid_16b& other );

            /// Convert the unique identifier to a hex string.
            /** @return A string representation of the id,
                    example: 1A57AA22-DCA5-48CE-81AC-651986ADF224 */
            t_string to_hex_string() const;

            /// Create a unique id from a serialized hex string.
            /** The passed string must match XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXX
                where X denotes a hexadecimal digit. */
            CORE_API friend uuid_16b from_hex_string( const t_string& hex_str );

            /// Compares for equality
            CORE_API friend bool operator==(const uuid_16b& lhs, const uuid_16b& rhs);

            /// Is lhs less than rhs?
            CORE_API friend bool operator<(const uuid_16b& lhs, const uuid_16b& rhs);

            /// Creates a text representation of this id.
            CORE_API friend t_ostream& operator<<( t_ostream& os, const uuid_16b& gid);

            /// Creates a unique id from a  text representation of this id.
            /** Calls from_hex_string internally. */
            CORE_API friend t_istream& operator>>( t_istream& is, uuid_16b& gid);

        private:
            void fill_with_unique_data();
            
            /// Private constructor to create an uninitialized uuid.
            uuid_16b( bool no_init );

            /// Used to parse uuids from strings.
            static t_regex m_uuid_regex;

            /// Used to output uuids to a string or stream.
            static t_format m_uuid_fmt;

            /// The actual unique data.
            boost::uint8_t m_data[16];
        };

    }
}

#endif // _CORE_UUID_16B_H_

