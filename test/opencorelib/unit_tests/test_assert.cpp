#include "precompiled_headers.hpp"
#include <boost/test/test_tools.hpp>


void asimple_function()
{
    ARASSERT_MSG(false, "Always fail. Not implemented!");
}

namespace Foo
{
    namespace Bar
    {
        void yafunction()
        {
            ARASSERT_MSG(false, "Always fail. Not implemented!");
        }

        class MyDummyClass
        {
        public:
            MyDummyClass()
            {
                // Test constructor site extraction in exception_context::AddContext.
                ARASSERT_MSG(false, "Always fail. Not implemented!");
            }
        };
    }
}

void test_assert()
{
//  try
//  {
//         Foo::Bar::MyDummyClass mdclass;
//         asimple_function();
//         Foo::Bar::yafunction();
// 
//      ARASSERT_MSG( false, "Hello %s!")("world!");
//         ARENFORCE_MSG(false, "A message %i")(5);
//      //ARENFORCE( false );
//         THROW_BASEEXCEPTION(_T("wholy macaronie"));
//  }
//  catch( const olib::useful::base_exception& e  )
//  {
// #ifdef DEBUG
//  //       olib::useful::user_reaction::type ua = olib::useful::base_exception::show(e);
//         if( ua == olib::useful::user_reaction::rethrow ) throw;
// #endif
//      //std::cout << "Catched CBaseException" << std::endl;
//  }
//  
}



