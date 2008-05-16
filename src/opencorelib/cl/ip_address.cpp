#include "precompiled_headers.hpp"
#include "./ip_address.hpp"
#include <boost/algorithm/string.hpp>
#include "./enforce_defines.hpp"
#include "./enforce.hpp"

namespace olib
{
	namespace opencorelib
	{
		ip_address::ip_address()
		{
			for(int i = 0; i < 4; ++i) m_address[i] = 0;
		}

		ip_address::ip_address( unsigned long addr)
		{
			m_address[0] = static_cast<unsigned char>((addr & 0xf000) >> 24);
			m_address[1] = static_cast<unsigned char>((addr & 0x0f00) >> 16);
			m_address[2] = static_cast<unsigned char>((addr & 0x00f0) >> 8);
			m_address[3] = static_cast<unsigned char>((addr & 0x000f)) ;
		}

		ip_address ip_address::from_string( const t_string& str)
		{
			std::vector<t_string> tokens; 
			boost::split( tokens, str, boost::is_any_of(_T(".")), boost::token_compress_on );
			ARENFORCE( tokens.size() == 4 )(str).msg(_T("Desired pattern: 255.255.255.255"));
			unsigned int value;
			ip_address address;
			for(int i = 0; i < 4; ++i)
			{
				t_stringstream ss(tokens[i]);
				ss >> value;
				ARENFORCE( value < 256).msg(_T("Invalid ip-address component."));
				address.m_address[i] = static_cast<unsigned char>(value);
			}
			return address;
		}

		ip_address::ip_address(unsigned char a0, unsigned char a1, unsigned char a2, unsigned char a3)
		{
			m_address[0] = a0;
			m_address[1] = a1;
			m_address[2] = a2;
			m_address[3] = a3;
		}

		t_string ip_address::as_string() const
		{
			t_format fmt(_T("%u.%u.%u.%u"));
			return (fmt % m_address[0] % m_address[1] % m_address[2] % m_address[3]).str();
		}

		unsigned int ip_address::to_uint() const
		{
			return (m_address[3]<<24) | (m_address[2]<<16) | (m_address[1]<<8) | m_address[0];
		}
		
		CORE_API bool operator==(const ip_address& lhs, const ip_address& rhs) 
		{
			return	lhs.m_address[0]==rhs.m_address[0] && 
					lhs.m_address[1]==rhs.m_address[1] && 
					lhs.m_address[2]==rhs.m_address[2] && 
					lhs.m_address[3]==rhs.m_address[3];
		}

		CORE_API bool operator!=(const ip_address& lhs, const ip_address& rhs) 
		{
			return !(lhs == rhs);
		}

		CORE_API bool operator<(const ip_address& lhs, const ip_address& rhs) 
		{
			if( lhs.m_address[0] < rhs.m_address[0] ) return true;
			if( lhs.m_address[0] > rhs.m_address[0] ) return false;
			if( lhs.m_address[1] < rhs.m_address[1] ) return true;
			if( lhs.m_address[1] > rhs.m_address[1] ) return false;
			if( lhs.m_address[2] < rhs.m_address[2] ) return true;
			if( lhs.m_address[2] > rhs.m_address[2] ) return false;
			if( lhs.m_address[3] < rhs.m_address[3] ) return true;
			if( lhs.m_address[3] > rhs.m_address[3] ) return false;
			return false; // equal
		}
	}
}
