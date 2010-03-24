#ifndef _CORE_ENFORCE_H_
#define _CORE_ENFORCE_H_

#ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
	#pragma warning( push )
	#pragma warning( disable : 4512 ) // Assignment operator could not be generated
	#pragma warning( disable : 4355 )
#endif

#include "./minimal_string_defines.hpp"
#include "./str_util.hpp"
#include <assert.h>

namespace olib
{
	namespace opencorelib
	{
        class exception_context;

		/// A class used in conjunction with the ARENFORCE macro.
		/** When the ARENFORCE macro is used and the tested expression 
			doesn't evaluate to true, an instance of this class is 
			created. The destructor of this class then throws an 
			exception. 
			@author Mats Lindel&ouml;f */
		class CORE_API invoke_enforce 
		{

		public:

			invoke_enforce& ARENFORCE_A;
			invoke_enforce& ARENFORCE_B;

			invoke_enforce();
			~invoke_enforce();

			invoke_enforce( const invoke_enforce & other);
			
			invoke_enforce& add_context(	const char* filename, long linenr, 
									const char* funcdname, const char* exp );

			invoke_enforce& msg(const char* m);
            invoke_enforce& msg(const wchar_t* m);
            invoke_enforce& msg(const utf8_string& m );
            invoke_enforce& msg(const utf16_string& m );

			invoke_enforce& hr( boost::int32_t hr);

			/// Set the machine readable reason for this exception to be thrown.
			/** Usage: aUENFORCE( exp ).reason(error::domain_error). 
				The error class contains a number of predefined error reason 
				strings that can be used if they describe the error correctly. 
				If no appropriate string is found, use your own and make sure 
				you document what it says. */
			invoke_enforce& reason(const TCHAR* r);


			template <class T>
			invoke_enforce& add_value( const char* exp, const T& obj)
			{
                if( ! m_exception_context ) return *this;
				ec_add_value( *m_exception_context ,exp, obj);
                ec_add_value_to_message( *m_exception_context, obj);
				return *this;
			}

			/// Will always return false.
			static bool always_fail();
			
		private:
			boost::shared_ptr<exception_context> m_exception_context;

		};

		inline invoke_enforce make_enforcer() 
		{
			if( str_util::env_var_exists(_CT("AML_USE_ASSERT_ARENFORCE")) )
			{
				//Don't throw an exception, just assert to break into the debugger
				assert( false && "ARENFORCE is implemented as assert since AML_USE_ASSERT_ARENFORCE is set" );
			}
			return invoke_enforce();
		}
	}	
}

#ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
	#pragma warning( pop )
#endif

#endif // _CORE_ENFORCE_H_

