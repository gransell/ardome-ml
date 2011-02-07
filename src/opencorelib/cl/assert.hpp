#ifndef _CORE_ASSERT_H_
#define _CORE_ASSERT_H_

#include "exception_context.hpp"
#include "basic_enums.hpp"

namespace olib
{
	namespace opencorelib
	{
		/// A class used in conjunction with the ARASSERT macro.
		/** When the expression inside an ARASSERT macro fails, an object of this
			type is created and information is added to it. When the object goes
			out of scope its destructor is invoked and the assertion is displayed 
			and possibly logged.
			@author Mats Lindel&ouml;f */
		class CORE_API invoke_assert
		{
		public:
			/// default constructor.
			invoke_assert(void);

			/// The destructor will display the assert.
			~invoke_assert(void);

			invoke_assert( const invoke_assert& o );
			invoke_assert& operator=(const invoke_assert& o);

			// These are necessary to make the preprocessor magic work.
			invoke_assert& ARASSERT_A;
			invoke_assert& ARASSERT_B;

			static invoke_assert make_assert();

			/// Set information on where the error occurred.
			invoke_assert& add_context( const char* filename, long linenr, 
								 const char* funcdname, const char* exp );
				
			/// Set the human readable error message.
            invoke_assert& msg(const char* m);
            invoke_assert& msg(const wchar_t* m);
            invoke_assert& msg(const utf8_string& m );
            invoke_assert& msg(const utf16_string& m );

			/// Set the machine readable error message.
			/** This message is supposed to be used to test if 
				an error is of a certain type. The idea is to 
				use string comparison instead of type matching to
				catch errors (which you normally do with exceptions) */
			invoke_assert& reason( const t_string& r);

			/// Set the severity of the assertion and a message.
			invoke_assert& level( assert_level::severity lvl , const t_string& str_msg = _CT(""));

			/// Set the severity to debug and the message to str_msg.
			invoke_assert& debug( const t_string& str_msg = _CT(""));

			/// Set the severity to warning and the message to str_msg.
			invoke_assert& warn( const t_string& str_msg = _CT(""));

			/// Set the severity to error and the message to str_msg.
			invoke_assert& error( const t_string& str_msg = _CT(""));

			/// Set the severity to fatal and the message to str_msg.
			invoke_assert& fatal( const t_string& str_msg = _CT(""));

			template< class T>
			invoke_assert & add_value(const char * variable_name, const T & val)
			{
				if( ! m_context ) return *this;
				ec_add_value(*m_context, variable_name, val);
                ec_add_value_to_message( *m_context, val);
				return *this;
			}

			/// Ignore this assertion or not.
			invoke_assert& ignore( bool b_ignore = true);

			/// Get the exception context of this assertion.
			boost::shared_ptr<exception_context> context();

			assert_level::severity level() const;

			/// Output this assertion to a stream.
			friend t_ostream& operator<<( t_ostream& ostrm, const invoke_assert& a);

			/// print this assertion for human interaction.
			void pretty_print( t_ostream& ostrm, print::option print_option );

			/// print this assertion for human interaction.
			void pretty_print_one_line( t_ostream& ostrm, print::option print_option);

			/// Get the current assert level as a string.
			t_string assert_level_asstring() const ;

			/// Will return false in debug build and true in release build.
			static bool always_fail();
			
			void operator++( )
			{
				// Silence compiler warning
			}
        
        private:
            assert_level::severity m_level;
            mutable bool m_ignore_this_assert;
            boost::shared_ptr<exception_context> m_context;

            void handle_assert();
		};
	}
}

#endif // _CORE_ASSERT_H_
