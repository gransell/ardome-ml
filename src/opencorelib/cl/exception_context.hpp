#ifndef _CORE_EXCEPTIONCONTEXT_H_
#define _CORE_EXCEPTIONCONTEXT_H_

#include "minimal_string_defines.hpp"
#include "basic_enums.hpp"

namespace olib
{
	namespace opencorelib
	{
		typedef std::vector< std::pair<t_string, t_string > >  name_value_container;
	
		template<class T>
		struct identity { typedef T type; };

		/// A class that stores information about the context where an assertion or exception occurred.
		/** This class is used by the assertions and exceptions in amf.
			Is stores information about where and why the exception/assertion occurred
			and a list of name and value pairs to help a human reader to figure 
			out the error reason. For computers a special string member, error_reason, 
			is used to handle exceptions. This string is used rather than creating new 
			exception types for all new errors.
			@author Mats Lindel&ouml;f*/
		class CORE_API exception_context
		{
		public:
			/// default constructor
			exception_context();

            exception_context(  const char* filename, long linenr, 
                                const char* funcdname, const char* exp, const TCHAR* msg );

			exception_context(	const char* filename, long linenr, 
								const t_string& source, const t_string& targetsite, 
								const char* exp);
			
			exception_context(	const char* filename, long linenr, 
								const t_string& source, const t_string& targetsite, 
								const char* exp, const t_string& message);

			exception_context(	const char* filename, long linenr, 
								const t_string& source, const t_string& targetsite, 
								const char* exp, const t_string& message, const t_string& callstack);


			exception_context(	const char* filename, long linenr, 
								const t_string& source, const t_string& targetsite, 
								const char* exp, const TCHAR* message);
			
			exception_context(	const char* filename, long linenr, 
								const t_string& source, const t_string& targetsite, 
								const char* exp, long hresult);

			exception_context(	const char* filename, long linenr, 
								const t_string& source, const t_string& targetsite, 
								const char* exp, long hresult, const t_string& message);

			exception_context(	const char* filename, long linenr, 
								const t_string& source, const t_string& targetsite, 
								const char* exp, long hresult, const TCHAR* message);


			~exception_context();

			exception_context( const exception_context& ec);
			exception_context& operator=(const exception_context& ec);
			
			/// Add the basic information where the error occurred.
			/** @param filename The name of the file where the error occurred.
				@param linenr The line number where the error occurred.
				@param funcdname A decorated function name, retreived by the preprocessor.
				@param exp The expression that caused the error */
			void add_context(const char* filename, long linenr, 
							const char* funcdname, const char* exp);


            /// Add a string value to the context
			void add_value( const char* exp, const t_string& val );

             /// Add a value to the message string
             /** Uses boost::format's % operator. 
                 @param t The value to add */
             template <class T>
             void add_value_to_message( const T& t)
             {
                 add_value_to_message(t, identity<T>( ) );
             }
			

			/// If the origin of the error was a COM-operation, the HRESULT is retrieved by this function.
			long h_result() const;
			/// Set the HRESULT if the error was caused by a COM-operation
			void h_result( long hr);

			/// Set the machine readable reason for this error.
			/** This string must be unique and preferably human readable as well.
				A suggestion is to use namespace qualifiers along with the object 
				name and then use a class unique constant at the end. 
				Ex: olib::opencorelib::str_util::impossible_conversion.
				This string should then be a public member of str_util so 
				users of str_util can catch exceptions and check the error_reason
				member of the base_exception for instance. */
			void error_reason( const t_string& er); 
			
			/// Set the callstack when this error occurred.
			void call_stack( const t_string& str);

			/// Get the callstack (if any) when this error occurred.
			t_string call_stack() const;

            /// Set the message.
            /** Can contain printf placeholders that will be 
                filled in by calls to add_value_to_message. */
            void message(const std::string& str);
            void message(const std::wstring& str);
            void message(const char* str);
            void message(const wchar_t* str);

			/// Write the current exception to the given stream using XML.
			t_ostream& as_xml( t_ostream& ss) const;

			/// Get a string representation in XML of this error.
			/**
				<code>
				&lt;exception_context&gt;<br>
				&lt;file&gt;...&lt;/file&gt;<br>
				&lt;line_nr&gt;...&lt;/line_nr&gt;<br>
				&lt;expression&gt;...&lt;/expression&gt;<br>
				&lt;source&gt;...&lt;/source&gt;<br>
				&lt;target_site&gt;...&lt;/target_site&gt;<br>
				&lt;message&gt;...&lt;/message&gt;<br>
				&lt;hresult&gt;...&lt;/hresult&gt;<br>
				&lt;machine_error&gt...&lt;/machine_error&gt
				&lt;context_values&gt;<br>
				&lt;name_value_pair name='...' value='...' /&gt;<br>
				...<br>
				&lt;context_values&gt;<br>
				&lt;/exception_context&gt;<br>
				</code>
				@return A t_string using the format outlined above.*/
			t_string as_xml() const;

			/// Prints this error in human readable format for display purposes.
			t_ostream& pretty_print( t_ostream& os, print::option print_empty_fields) const;
			
			/// Prints this error in human readable format for display purposes.
			t_string pretty_print(print::option print_empty_fields) const;

			/// Prints this error in human readable format for display purposes.
			t_ostream& pretty_print_one_line( t_ostream& os, print::option print_empty_fields) const;

			/// Output this error to the given stream.
			friend t_ostream& operator << ( t_ostream& os, exception_context& exp);

			/// Get the human readable error message for this error.
			t_string message() const;

			/// Get the filename where this error occured.
			const t_string& filename() const;

			/// Get the name of the object where this error was caused.
			const t_string& source() const;

			/// Set the source of this exception.
			void source( const t_string& s);

			/// Get the name of the function where this error was caused.
			const t_string& target_site() const;

			/// Get the expression that caused this error.
			const t_string& expression() const;

			/// Get the line number in the file where this error was caused.
			long line_nr() const;

			/// Get the machine readable reason for this error.
			/** Check the documentation for each function that throws an 
				exception how this string is created. If the thrower doesn't
				provide a machine readable error this string is empty. */
			const t_string error_reason() const;

			/// Swap to contexts.
			void swap( exception_context& other) throw();
    
        private:
            static void print_value_pair( const std::pair< t_string, t_string >& p, t_ostream& ss);
            static void pretty_print_value_pair( const std::pair< t_string, t_string >& p, t_ostream& ss);
            static void pretty_print_value_pair_one_line( const std::pair< t_string, t_string >& p, t_ostream& ss);
            t_string m_call_stack;
            t_string m_error_reason;
            t_string m_expression;
            t_string m_file_name;
            long m_hr;
            long m_line_nr;
            t_format m_message;
            name_value_container m_name_value_vec;
            t_string m_source;
            t_string m_target_site;

            template <class T>
            void add_value_to_message( const T& t, identity<T>)
            {
                m_message % t;
            }

            void add_value_to_message( const char* t, identity<const char*> )
            {
                if ( t == 0 ) {
                    m_message % "( NULL )";
                } else {
                    m_message % t;
                }
            }

            void add_value_to_message( char* t, identity<char*> )
            {
                if ( t == 0 ) {
                    m_message % "( NULL )";
                } else {
                    m_message % t;
                }
            }

		};

        template <class T>
        void ec_add_value(    exception_context& ec, const char* exp, const T& obj )
        {
            t_stringstream valstream;
            valstream << obj;
            ec.add_value(exp, valstream.str());
        }

        /// Specialization for char pointers.
        CORE_API void ec_add_value( exception_context& ec, const char* exp, const char* obj );

        /// Specialization for wchar_t pointers.
        CORE_API void ec_add_value( exception_context& ec, const char* exp, const wchar_t* & obj );

        /// Specialization for strings. 
        CORE_API void ec_add_value( exception_context& ec, const char* exp, const std::wstring& obj);

        CORE_API void ec_add_value( exception_context& ec, const char* exp, const std::string& obj);

        template <class T>
        void ec_add_value_to_message( exception_context& ec, const T& obj )
        {
            ec.add_value_to_message(obj);
        }

        /// Specialization for char pointers.
        CORE_API void ec_add_value_to_message( exception_context& ec, const char* obj );

        /// Specialization for wchar_t pointers.
        CORE_API void ec_add_value_to_message( exception_context& ec, const wchar_t* obj );

        /// Specialization for strings. 
        CORE_API void ec_add_value_to_message( exception_context& ec, const std::wstring& obj);

        CORE_API void ec_add_value_to_message( exception_context& ec, const std::string& obj);
	}
}

#endif // _CORE_EXCEPTIONCONTEXT_H_

