#ifndef _CORE_STING_CONVERSIONS_H_
#define _CORE_STING_CONVERSIONS_H_

/// @file string_conversions.h
/** Functions to convert between ansi and unicode. */
#include "./minimal_string_defines.hpp"

namespace olib
{
	namespace opencorelib
	{
		namespace string_conversions
		{
            /** Makes sure that a string is represented as a 2-byte string. On some Unix systems wchar_t
                is 4 bytes which causes problems for some conversion methods. This method will always copy the
                bytes so if you know that your system has 2 byte wchars then it is not necessary to call this method.
                @param unpacked The potentially unpacked string.
                @param num_chars The number of characters to convert.
                @returns A vector containing the 2-byte chars. */
            std::vector<boost::uint16_t> pack_wide_string( const boost::uint32_t* const unpacked, const size_t num_chars );
			
			/**
			    Converts a 2-byte string to a wchar_t string. On some systems wchar_t is 4 bytes so we need to widen
			    the 2-bytes string.
			    @param packed Pointer to the 2-byte string to be widened.
			    @param num_chars Number of characters in the string.
			    @returns A vector containing the widened characters. */
			std::vector<wchar_t> unpack_packed_wide_string( const boost::uint16_t* const packed, const size_t num_chars );
			
            utf8_string from_2b_utf16_to_utf8( const boost::uint16_t* const src, size_t src_size );
            
			/// Convert a wide UTF-16 string to a UTF-8 string
			utf8_string from_utf16_to_utf8( const wchar_t* src, size_t src_size );
		
			/// Convert a UTF-8 string to a UTF-16 string
			utf16_string from_utf8_to_utf16( const char* src, size_t src_size );
		}
			
	}
}

#endif // _CORE_STING_CONVERSIONS_H_
