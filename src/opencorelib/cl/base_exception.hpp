#ifndef _CORE_BASE_EXCEPTION_H_
#define _CORE_BASE_EXCEPTION_H_

#include "exception_context.hpp" 
namespace olib
{
	namespace opencorelib
	{
		namespace user_reaction
		{
			enum  type {ignore, rethrow};
		}

		/// Base for assert and enforce exceptions 
		/**	Use this class or derive from this class when creating and 
			throwing exceptions.
			@author Mats Lindel&ouml;f*/
		class CORE_API base_exception : public std::exception
		{
		public:

			/// This only available constructor demands that the user provides all parameters.
			/**
			@param context The context in which the exception was thrown.*/
			base_exception(const exception_context& context );
			
			/// The default destructor. Empty implementation
			virtual ~base_exception(void) throw();	

			/// Overridden function in std::exception
			virtual const char* what( ) const throw( );

			/// Get a description of this exception in XML
			/** @return A xml representation of the exception.*/
			t_string as_xml() const;

			/// Sets the name of the original exception type.
			/** If an exception is caught and transformed into a CBase_exception the type of 
			    the original exception should be recorded here.
			    @param type_name The name of the original exception type. */
            void original_exception_type(const t_string& type_name);

			/// Gets the name of the original exception type.
			/** @return A string containing the name of the original exception type (if it was set).*/
			const t_string& original_exception_type(void) const;

			/// Sets the function name where the original exception was caught and transformed into a CBase_exception.
			/** @param function_name The name of the function where the original exception was caught.*/
			void transformation_function(const t_string& function_name);

			/// Gets the function name where the original exception was caught and transformed into a CBase_exception.
			/**
			@return The name of the function where the original exception was caught.
			*/
			const t_string& transformation_function(void) const;

			/// If a CBase_exception is caught and rethrown, the name of the catching function should be recorded by this function.
			/**
			@param function_name The name of the function where this object was caught and rethrown.
			*/
			void add_function_to_catch_stack(const t_string& function_name);

			/// Retreives the whole catch stack as a vector of strings.
			/**
				The oldest entry in the vector is the first. 
				@return A std::vector with t_strings where each string is a function where this exception was caught and rethrown.
			*/
			const std::vector<t_string>& catch_stack(void) const;

			const exception_context& context() const;
			exception_context& context();

			/// Show exception in a dialog box

			static user_reaction::type show( const base_exception& e);
			static user_reaction::type show( const base_exception& e, const t_string& caption);
			void pretty_print( t_ostream& ostrm, print::option print_option ) const;
			void pretty_print_one_line(t_ostream& ostrm, print::option print_option) const;

		private:

			static void print_catch_stack(t_stringstream& os, const t_string& func_name);
			static void pretty_print_catch_stack(t_ostream& os, const t_string& func_name, bool insert_new_line);
		
			std::vector<t_string> m_catch_stack;
			t_string m_original_exception_type;
			t_string m_transformation_function;
			mutable std::string m_std_except_message;
			
			exception_context m_context;
		};
	}
}

#endif // _CORE_BASE_EXCEPTION_H_

