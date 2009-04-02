#ifndef _CORE_THROW_TRY_CATCH_DEFINES_H_
#define _CORE_THROW_TRY_CATCH_DEFINES_H_

/// Throws a base_exception and fills it in with information concerning the current context, using the __FUNCDNAME__ macro.
/**
@param hres The error code which caused the need to throw the exception
@param msg A message that will be added to the CBase_exeption which can be read later on by catch handlers.
*/
#define THROW_BASEEXCEPTION(msg) throw olib::opencorelib::base_exception \
    (olib::opencorelib::exception_context( __FILE__, __LINE__, OLIB_CURRENT_FUNC_NAME, "unknown expression", msg));

#define EXCEPTION_CONTEXT(msg) \
    (olib::opencorelib::exception_context( __FILE__, __LINE__, OLIB_CURRENT_FUNC_NAME, "unknown expression", msg))


/// Used to encapsulate all C++ functions.
/**
Used in conjunction with cPPCATCH().
@sa CPPCATCH
*/
#define CPPTRY try 

/// Only used in pure C++ functions to catch all exception that could be thrown
/**
Used in conjunction with CPPTRY.
Converts all known exceptions into base_exceptions. Then the exception is rethrown. 
If the caught exception already is a base_exception the current catch-handler's context will 
be added to the exception's catch-stack. The catch-stack could then be used as an aid to 
detect where the original error occurred.
@sa CPPTRY
*/
#define CPPCATCH_RETHROW() \
	catch (olib::opencorelib::base_exception& ex) {\
	    ex.add_function_tocatch_stack(OLIB_CURRENT_FUNC_NAME);\
	    throw ex;\
	}\
	catch (std::exception& ex) {\
	    olib::opencorelib::base_exception new_ex \
            (olib::opencorelib::exception_context(__FILE__, __LINE__, OLIB_CURRENT_FUNC_NAME , \
                                          "unknown expression.", -1 )); \
	    new_ex.context().add_message(olib::opencorelib::str_util::to_t_string(utf8_string(ex.what()))); \
	    new_ex.original_exception_type(_CT("std::exception"));\
	    throw new_ex; \
	}\
	catch (...) {\
	    olib::opencorelib::base_exception new_ex(olib::opencorelib::exception_context(__FILE__, __LINE__, \
                                                                      OLIB_CURRENT_FUNC_NAME , \
                                                                      "unknown expression.", -1 )); \
	    new_ex.original_exception_type(_CT("unknown exception (access violation or some other nasty thing)"));\
	    throw new_ex; \
	}

#define TRY_BASEEXCEPTION try 

#ifndef E_FAIL
	#define E_FAIL -1
#endif

#ifdef _DEBUG
    #define CATCH_BASEEXCEPTION() \
        catch(olib::opencorelib::base_exception& e){ olib::opencorelib::base_exception::show(e); } \
        catch( std::exception& e) { ARASSERT(false)(e.what()); }

    #define CATCH_BASEEXCEPTION_RETURN(to_return) \
        catch(olib::opencorelib::base_exception& e){ olib::opencorelib::base_exception::show(e); return to_return; } \
        catch( std::exception& e) { ARASSERT(false)(e.what()); return to_return; }

    #define CATCH_BASEEXCEPTION_RETURN_HR() \
        catch(olib::opencorelib::base_exception& e) \
            { olib::opencorelib::base_exception::show(e); return e.context().h_result(); } \
        catch( std::exception& ) { return E_FAIL; }
#else
	#define CATCH_BASEEXCEPTION()  catch(olib::opencorelib::base_exception& ){} \
									catch( std::exception& ) {}

    #define CATCH_BASEEXCEPTION_RETURN(to_return)                \
        catch(olib::opencorelib::base_exception& ) { return to_return; } \
        catch( std::exception& ) { return to_return; }

    #define CATCH_BASEEXCEPTION_RETURN_HR()                                 \
        catch(olib::opencorelib::base_exception& e) { return e.context().h_result(); }   \
        catch( std::exception& ) { return E_FAIL; }
#endif

#endif // _CORE_THROW_TRY_CATCH_DEFINES_H_

