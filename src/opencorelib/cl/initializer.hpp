#ifndef _CORE_INITIALIZER_H_
#define _CORE_INITIALIZER_H_

namespace olib
{
	namespace opencorelib
	{
		/// This class forces the compiler to run the do_premain_register function.
		/** This class is part of a trick. The trick goes like this:
			In the constructor of this class a function in another class
			is called. This function is static and called Do_premain_register.
			The idea is that the other class should keep at static member
			of this class inside it. But since we're dealing with templates
			this is not enough to make the compiler create the necessary code.
			Therefore the foo function is there. This function is called 
			in the default constructor of the hosting class. Since it is 
			empty an efficient compiler will be able to remove it completely.
			But, the template instanciator doesn't know foo is meaningless, 
			therefore it has to create the initializer instance which in 
			turn runs whatever startup code we want automatically, when 
			the c-runtime is started that is.

            Make sure that inheriting classes always provide at least one
            constructor (not only the automatically generated).
			
			@author Mats Lindel&ouml;f*/
		template <class T>
		class initializer
		{
		public:
			initializer()
			{
				T::do_premain_register();
			}

			~initializer()
			{
				T::do_final_unregister();
			}

			inline void foo() const {}
		};

	}
}

#endif //_CORE_INITIALIZER_H_

