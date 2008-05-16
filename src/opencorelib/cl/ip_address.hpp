#ifndef _CORE_IP_ADDRESS_H_
#define _CORE_IP_ADDRESS_H_

namespace olib
{
	namespace opencorelib
	{
		/// Description of an IP-address
		/** @author Mats Lindel&ouml;f*/
		class CORE_API ip_address
		{
		private:
			unsigned char m_address[4];
		public:
			/// Construct an IP-address, set to 0.0.0.0 initially.
			ip_address();

			/// Construct an IP-address from a long value.
			/** @param ip The value is masked like this:
				[aaaaaaaabbbbbbbbccccccccdddddddd]. Each small letter represents a bit.
				The a-bits are masked and stored in the first component of the address,
				the b-bits in the second and so forth.  */
			explicit ip_address( unsigned long ip);

			/// Constructs an IP-address from four unsigned char values.
			ip_address(unsigned char a0, unsigned char a1, unsigned char a2, unsigned char a3);

			/// Constructs an IP-address from a string that should look like this "222.195.2.32"
			/** If parsing of the string fails a base_exception is thrown. */
			static ip_address from_string( const t_string& str);

			/// Get a string representation of the address.
			t_string as_string() const;

			/// Convert the address to an unsigned integer value. 
            unsigned int to_uint() const;

			/// Equality operator.
			CORE_API friend bool operator==(const ip_address& lhs, const ip_address& rhs);
			/// Inequality operator.
			CORE_API friend bool operator!=(const ip_address& lhs, const ip_address& rhs);
			/// Less than operator.
			CORE_API friend bool operator<(const ip_address& lhs, const ip_address& rhs);

		};
	}
}

#endif // _CORE_IP_ADDRESS_H_

