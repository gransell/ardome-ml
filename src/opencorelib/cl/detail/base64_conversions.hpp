#ifndef BASE64_CONVERSIONS_H_INCLUDED_
#define BASE64_CONVERSIONS_H_INCLUDED_

#include "opencorelib/cl/core.hpp"
#include "opencorelib/cl/enforce_defines.hpp"


namespace olib{ namespace opencorelib{ namespace detail{ 

	const char base64_enc_lookup[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	const boost::uint8_t base64_dec_lookup[80] = {
		0x3E, // '+'
		0,0,0,
		0x3F, // '/' 
		0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D, //0-9
		0,0,0,0,0,0,0,
		0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F, //A-Z
		0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,
		0,0,0,0,0,0,
		0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29, //a-z
		0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,0x30,0x31,0x32,0x33 };

    inline bool is_whitespace( char c )
    {
        switch(c)
        {
            case ' ':
            case '\n':
            case '\t':
                return true;
            default:
                return false;
        }
    }

	template<typename T, typename U>
	void base64_encode( T start, T end, U output )
	{
		T iter = start;
		
		boost::uint8_t buf[3];
	
		while( true )
		{
			for(int i=0; i<3; i++)
			{
				if( iter != end )
				{
					buf[i] = *iter++;
				}
				else
				{
					for( int j=i; j<3; j++ )
					{
						buf[j] = 0;
					}
					
					if( i >= 1 )
					{
						(*output++) = ( base64_enc_lookup[buf[0] >> 2] );
						(*output++) = ( base64_enc_lookup[((buf[0] & 0x03) << 4) | ((buf[1] & 0xF0) >> 4)] );
						if( i >= 2 )
						{
							(*output++) = ( base64_enc_lookup[((buf[1] & 0x0F) << 2) | ((buf[2] & 0xC0) >> 6)] );
						}
						else
						{
							(*output++) = '=';
						}

						(*output++) = '=';
					}

					return;
				}
			}
	
			(*output++) = ( base64_enc_lookup[buf[0] >> 2] );
			(*output++) = ( base64_enc_lookup[((buf[0] & 0x03) << 4) | ((buf[1] & 0xF0) >> 4)] );
			(*output++) = ( base64_enc_lookup[((buf[1] & 0x0F) << 2) | ((buf[2] & 0xC0) >> 6)] );
			(*output++) = ( base64_enc_lookup[buf[2] & 0x3F] );
		}
	}

	template<typename T, typename U>
	void base64_decode( T start, T end, U output )
	{
		T iter = start;
	
		boost::uint8_t buf[4];

        while( true )
        {
            for( int i=0; i<4; )
            {
				if( iter == end )
				{
					ARENFORCE_MSG( i == 0, "base64 encoded data has a character count that is not a multiple of 4" );
					return;
				}

                char val = (*iter++);
                if( is_whitespace( val ) )
                    continue;

                if( val == '=' )
                {
                    ARENFORCE_MSG( i >= 2, "Incorrectly placed '=' in base64 encoded data!" );

                    if( i == 2 )
                    {
                        while( iter != end && is_whitespace( *iter ) )
                            ++iter;

                        ARENFORCE_MSG( iter != end, "Incorrectly placed '=' in base64 encoded data!" );

                        ARENFORCE_MSG( (*iter) == '=', "'=' followed by illegal character '%1%' in base64 encoded data detected!" )( *iter );

                        //Only first byte is valid
                        (*output++) = ( (buf[0] << 2) | ((buf[1] & 0x30) >> 4) );
                    }
                    else
                    {
                        (*output++) = ( (buf[0] << 2) | ((buf[1] & 0x30) >> 4) );
                        (*output++) = ( ((buf[1] & 0x0F) << 4) | ((buf[2] & 0x3C) >> 2) );
                    }

                    return;
                }
                else
                {
					//Simplified test for efficiency, as there are a few characters in this
					//interval that are not valid in base64 encoding.
					ARENFORCE_MSG( val >= 43 && val <= 122, "Invalid character in base64 encoded data, code: %1%" )( (int)val );
                    buf[i] = base64_dec_lookup[val - 43];
                }

                ++i;
            }

            (*output++) = ( (buf[0] << 2) | ((buf[1] & 0x30) >> 4) );
            (*output++) = ( ((buf[1] & 0x0F) << 4) | ((buf[2] & 0x3C) >> 2) );
            (*output++) = ( ((buf[2] & 0x03) << 6) | buf[3] );
        }
    }

} } }

#endif //#ifndef BASE64_CONVERSIONS_H_INCLUDED_

