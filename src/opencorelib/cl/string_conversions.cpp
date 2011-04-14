
#include "precompiled_headers.hpp"
#include "./string_conversions.hpp"
#include "./xerces_typedefs.hpp"

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/OutOfMemoryException.hpp>
#include <xercesc/util/TransService.hpp>

#define LOKI_CLASS_LEVEL_THREADING
#include <loki/Singleton.h>

#include <iostream>

using namespace XERCES_CPP_NAMESPACE;

namespace olib
{
	namespace opencorelib
	{
		namespace string_conversions 
		{
			namespace {   

				struct init_xerces
				{
					bool m_b_initialized;
					init_xerces() : m_b_initialized(false)
					{
					}
					
					bool init()
					{
						try
					    {
                            if(m_b_initialized) return true;
						    XMLPlatformUtils::Initialize();
						    m_b_initialized = true;
						    return true;
					    }
						catch (const XMLException& )
					    {
							std::cerr << "Could not initialize xerces" << std::endl;
							return false;
					    }
					}
					
					~init_xerces()
					{
						// Make sure we clean up after we're done.
						if( m_b_initialized )
						{
							XMLPlatformUtils::Terminate();
							m_b_initialized = false;
						}
					}
				};
				
				bool create_xerces()
				{
					static boost::shared_ptr< init_xerces > init_obj;
					if( init_obj ) return init_obj->m_b_initialized ;
					
					init_obj = boost::shared_ptr< init_xerces >(new init_xerces());
					return init_obj->init();
				}

                 static bool force_xerxec_initialization = create_xerces();
			}
			
			std::vector<boost::uint16_t> pack_wide_string(  const boost::uint32_t* const unpacked, 
                                                            const size_t num_chars )
			{
				std::vector<boost::uint16_t> ret( num_chars + 1 );
				size_t i = 0;
				while( i < num_chars ) {
					ret[i] = (boost::uint16_t)unpacked[i];
					++i;
				}
                ret[num_chars] = 0;
                
				return ret;
			}
			
			
			std::vector<wchar_t> unpack_packed_wide_string( const boost::uint16_t* const packed, 
                                                            const size_t num_chars )
			{
				std::vector<wchar_t> ret( num_chars + 1 );
				size_t i = 0;
				while( i < num_chars ) {
					ret[i] = (wchar_t)packed[i];
					++i;
				}
                ret[num_chars] = 0;
				return ret;
			}

			class amf_utf_transcoder
			{
			public:
				amf_utf_transcoder()
				{
					if( !create_xerces() ) 
					{
						std::cerr << "Failed to initialize xerces" << std::endl;
					}

					if( !XMLPlatformUtils::fgTransService ) 
					{
						// We can't use our standard assertions and stuff here since that will trigger a
						// utf8 -> utf16 convert and we will end up here again
						std::cerr << "No transcoder service available. Is xerces initialized?" << std::endl;
					}

					XMLTransService::Codes v;
					unsigned int bufSize = 16 * 1024;
					
					m_utf8_transcoder = 
						boost::shared_ptr<XMLTranscoder>(	XMLPlatformUtils::fgTransService->
						makeNewTranscoderFor(XMLRecognizer::UTF_8, v, bufSize));

					if( v != XMLTransService::Ok ) 
					{
						std::cerr << "Failed to get UTF-8 transcoder" << v << std::endl;
						m_utf8_transcoder.reset();
					}
				}

				boost::shared_ptr<XMLTranscoder> get_xerces_utf8_transcoder() const 
				{ 
					return m_utf8_transcoder;
				}

			private:

				boost::shared_ptr<XMLTranscoder> m_utf8_transcoder;

			};

			typedef Loki::SingletonHolder<	amf_utf_transcoder > the_amf_utf_transcoders;
			
			utf8_string from_2b_utf16_to_utf8( const boost::uint16_t* const src, size_t src_size )
			{
				try
				{
					// The utf8 transcoder will convert to and from the 
					// internal xerces representation (utf16).
					boost::shared_ptr<XMLTranscoder> trans = 
						the_amf_utf_transcoders::Instance().get_xerces_utf8_transcoder();
					if(!trans) 
					{
						std::cerr << "Failed to get a transcoder to convert from utf16 to utf8\n"; 
						return utf8_string();
					}

					std::vector< boost::uint8_t> buf( src_size * 6);
					xerces_size_type chars_eaten(0);
					unsigned int target_size = 
						trans->transcodeTo(   (const XMLCh*)src, 
												static_cast<xerces_size_type>(src_size), &buf[0], 
												static_cast<xerces_size_type>(buf.size()), 
												chars_eaten, XMLTranscoder::UnRep_Throw);
					if( chars_eaten != static_cast<unsigned int>(src_size) ) 
					{
						std::stringstream ss;
						ss << "UTF-16 -> UTF-8 conversion failed chars_eaten=" << chars_eaten
							<< " src_size=" << static_cast<unsigned int>(src_size);
						throw std::runtime_error( ss.str().c_str() );
					}
					return utf8_string( (char*)&buf[0], target_size );
				}
				catch ( XMLException& e )
				{
					#ifdef OLIB_ON_WINDOWS
						std::wcerr << L"XMLExeption::getMessage: " << e.getMessage() << std::endl;
					#endif
					throw std::runtime_error( "UTF-16 -> UTF-8 conversion failed" );
				}
			}	
			
			utf8_string from_utf16_to_utf8( const wchar_t* src, size_t src_size )
			{
				if( src_size == 0 ) {
					return utf8_string();
				}

				const boost::uint16_t* input = (const boost::uint16_t*)src;
				
                #if defined( OLIB_ON_MAC ) || defined( OLIB_ON_LINUX )
                    std::vector<boost::uint16_t> packed_input;
                    // Since wchar_t is 4 bytes on OS X we need to reduce it to 2 before we pass it to
                    // transcode
                    packed_input = pack_wide_string( reinterpret_cast<const boost::uint32_t*>(src),
                                                     src_size );
                    input = &packed_input[0];
                #endif
				
				return from_2b_utf16_to_utf8( input, src_size );
			}
			
			utf16_string from_utf8_to_utf16( const char* src, size_t src_size )
			{
				if( src_size == 0 ) return utf16_string();
				
				try
				{
					boost::shared_ptr<XMLTranscoder> trans = 
						the_amf_utf_transcoders::Instance().get_xerces_utf8_transcoder();

					if(!trans) {
						std::cerr << "Failed to get a transcoder to convert from utf8 to utf16\n"; 
						return utf16_string();
					}

					std::vector< boost::uint8_t > char_sizes(src_size);
					std::vector< boost::uint16_t > buf( src_size );

					xerces_size_type chars_eaten(0);
					unsigned int target_size = 
						trans->transcodeFrom( (const XMLByte*)src, 
                                           		static_cast<xerces_size_type>(src_size), (XMLCh*)&buf[0], 
                                           		static_cast<xerces_size_type>(buf.size()), 
										   		chars_eaten, &char_sizes[0] );
					
					if( chars_eaten != src_size ) {
						std::stringstream ss;
						ss << "UTF-8 -> UTF-16 conversion failed chars_eaten=" << chars_eaten
                           << " src_size=" << static_cast<unsigned int>(src_size) << " src=" << std::string(src, src_size);
						throw std::runtime_error( ss.str().c_str() );
					}
					
					const wchar_t* output = (const wchar_t*)&buf[0];
					std::vector<wchar_t> unpacked_output;
					if( sizeof(wchar_t) == 4 ) {
						unpacked_output = unpack_packed_wide_string( &buf[0], chars_eaten );
						output = &unpacked_output[0];
					}
					
					return utf16_string( output, target_size );
				}
				catch ( XMLException& e)
				{
					xerces_string str( e.getMessage() );
					// We can't use our standard assertions and stuff here since that will trigger a utf8 ->
					// utf16 convert and we will end up here again
					#ifdef OLIB_ON_WINDOWS
						std::wcerr << L"XMLExeption::getMessage: " << e.getMessage() << std::endl;
					#else	
						char *ptr = const_cast< char * >( reinterpret_cast< const char * >( str.c_str( ) ) );
						for( size_t i = 0; i < str.size( ) * 2; ++i )
						{
							if( ptr[ i ] == 0 ) ptr[ i ] = '.';
						}
						std::cerr <<  "XMLExeption::getMessage: " << ptr << std::endl;
					#endif
					std::string err_msg(src, src_size);
					err_msg = "UTF-8 -> UTF-16 conversion failed src=" + err_msg;
					throw std::runtime_error( err_msg.c_str() );
				}
			}			
		}
	}
}
