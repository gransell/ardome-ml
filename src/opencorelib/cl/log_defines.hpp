#ifndef _CORE_LOG_DEFINES_H_
#define _CORE_LOG_DEFINES_H_

#include "basic_enums.hpp"

/** @file log_defines.h */

/** @def ARLOG 
    Use this macro to always log something. This means that 
    the log request is always passed on to all log_targets registered
    with the_log_handler, what each log_target is doing with the
    request is another question, they might choose to ignore 
    the request or not. 

    To set the log-level (that can be inspected by each log_target),
    use the following syntax:
    <pre>
    ARLOG("My message").level(log_level::debug1);
    </pre>

    To inject variable values in the message string, use the
    following syntax:
    <pre> 
    ARLOG("My msg: i = %i and str= %s")(i)(str);
    // To set the level at the same time:
    ARLOG("My msg: i = %i and str= %s")(i)(str).level(log_level::debug1);
    </pre>

    You can put as many parentheses as you like after the ARLOG macro,
    each pair will add a value to the format string 
    used in the msg parameter. 

    <pre>
    // If you are used to printf syntax, something that would look like this:
    sprintf( buf, "My msg: i = %i and str= %s", i, str);

    // would be translated to this:
    ARLOG("My msg: i = %i and str= %s")(i)(str);
    </pre>

    @sa ARLOG_IF
    @author Mats Lindel&ouml;f*/
#define ARLOG(the_msg) \
    ++olib::opencorelib::logger::make_logger() \
    .add_context(__FILE__, __LINE__, OLIB_CURRENT_FUNC_NAME).msg(the_msg).ARLOG_A

#define ARSCOPELOG_MAKER( trace_id, msg, lvl ) \
    boost::shared_ptr< olib::opencorelib::scope_logger> trace_id; \
	if( ARCOND_LEVEL(lvl) ) \
        trace_id = boost::shared_ptr<olib::opencorelib::scope_logger> \
        ( new olib::opencorelib::scope_logger( msg, lvl, __FILE__, __LINE__,  OLIB_CURRENT_FUNC_NAME, __FUNCTION__ ) )\

#define ARSCOPELOG_LEVEL( lvl ) ARSCOPELOG_MAKER( ARMAKE_UNIQUE_NAME(trace_log), "", lvl )

#define ARSCOPELOG_MSG_LEVEL( msg, lvl ) ARSCOPELOG_MAKER( ARMAKE_UNIQUE_NAME(trace_log), msg, lvl )

#define ARSCOPELOG_MSG_INFO( msg ) ARSCOPELOG_MAKER( ARMAKE_UNIQUE_NAME(trace_log), msg, olib::opencorelib::log_level::info );
#define ARSCOPELOG_MSG_DEBUG( msg ) ARSCOPELOG_MAKER( ARMAKE_UNIQUE_NAME(trace_log), msg, olib::opencorelib::log_level::debug1 );
#define ARSCOPELOG_MSG_DEBUG2( msg ) ARSCOPELOG_MAKER( ARMAKE_UNIQUE_NAME(trace_log), msg, olib::opencorelib::log_level::debug2 );
#define ARSCOPELOG_MSG_DEBUG3( msg ) ARSCOPELOG_MAKER( ARMAKE_UNIQUE_NAME(trace_log), msg, olib::opencorelib::log_level::debug3 );
#define ARSCOPELOG_MSG_DEBUG4( msg ) ARSCOPELOG_MAKER( ARMAKE_UNIQUE_NAME(trace_log), msg, olib::opencorelib::log_level::debug4 );
#define ARSCOPELOG_MSG_DEBUG5( msg ) ARSCOPELOG_MAKER( ARMAKE_UNIQUE_NAME(trace_log), msg, olib::opencorelib::log_level::debug5 );
#define ARSCOPELOG_MSG_DEBUG6( msg ) ARSCOPELOG_MAKER( ARMAKE_UNIQUE_NAME(trace_log), msg, olib::opencorelib::log_level::debug6 );
#define ARSCOPELOG_MSG_DEBUG7( msg ) ARSCOPELOG_MAKER( ARMAKE_UNIQUE_NAME(trace_log), msg, olib::opencorelib::log_level::debug7 );
#define ARSCOPELOG_MSG_DEBUG8( msg ) ARSCOPELOG_MAKER( ARMAKE_UNIQUE_NAME(trace_log), msg, olib::opencorelib::log_level::debug8 );

#define ARSCOPELOG_INFO() ARSCOPELOG_MAKER( ARMAKE_UNIQUE_NAME(trace_log), "", olib::opencorelib::log_level::info )
#define ARSCOPELOG_DEBUG() ARSCOPELOG_MAKER( ARMAKE_UNIQUE_NAME(trace_log), "", olib::opencorelib::log_level::debug1 )
#define ARSCOPELOG_DEBUG2() ARSCOPELOG_MAKER( ARMAKE_UNIQUE_NAME(trace_log), "", olib::opencorelib::log_level::debug2 )
#define ARSCOPELOG_DEBUG3() ARSCOPELOG_MAKER( ARMAKE_UNIQUE_NAME(trace_log), "", olib::opencorelib::log_level::debug3 )
#define ARSCOPELOG_DEBUG4() ARSCOPELOG_MAKER( ARMAKE_UNIQUE_NAME(trace_log), "", olib::opencorelib::log_level::debug4 )
#define ARSCOPELOG_DEBUG5() ARSCOPELOG_MAKER( ARMAKE_UNIQUE_NAME(trace_log), "", olib::opencorelib::log_level::debug5 )
#define ARSCOPELOG_DEBUG6() ARSCOPELOG_MAKER( ARMAKE_UNIQUE_NAME(trace_log), "", olib::opencorelib::log_level::debug6 )
#define ARSCOPELOG_DEBUG7() ARSCOPELOG_MAKER( ARMAKE_UNIQUE_NAME(trace_log), "", olib::opencorelib::log_level::debug7 )
#define ARSCOPELOG_DEBUG8() ARSCOPELOG_MAKER( ARMAKE_UNIQUE_NAME(trace_log), "", olib::opencorelib::log_level::debug8 )

/// Convenience macro for filtering and logging at the emergency level
#define ARLOG_EMERGENCY(the_msg) ARLOG_IF_LEVEL( olib::opencorelib::log_level::emergency, the_msg )
/// Convenience macro for filtering and logging at the alert level
#define ARLOG_ALERT(the_msg) ARLOG_IF_LEVEL( olib::opencorelib::log_level::alert, the_msg )
/// Convenience macro for filtering and logging at the critical level
#define ARLOG_CRIT(the_msg) ARLOG_IF_LEVEL( olib::opencorelib::log_level::critical, the_msg )
/// Convenience macro for filtering and logging at the error level
#define ARLOG_ERR(the_msg) ARLOG_IF_LEVEL( olib::opencorelib::log_level::error, the_msg )
/// Convenience macro for filtering and logging at the warning level
#define ARLOG_WARN(the_msg) ARLOG_IF_LEVEL( olib::opencorelib::log_level::warning, the_msg )
/// Convenience macro for filtering and logging at the notice level
#define ARLOG_NOTICE(the_msg) ARLOG_IF_LEVEL( olib::opencorelib::log_level::notice, the_msg )
/// Convenience macro for filtering and logging at the info level
#define ARLOG_INFO(the_msg) ARLOG_IF_LEVEL( olib::opencorelib::log_level::info, the_msg )
/// Convenience macro for filtering and logging at the debug1 level
#define ARLOG_DEBUG(the_msg) ARLOG_IF_LEVEL( olib::opencorelib::log_level::debug1, the_msg )
/// Convenience macro for filtering and logging at the debug2 level
#define ARLOG_DEBUG2(the_msg) ARLOG_IF_LEVEL( olib::opencorelib::log_level::debug2, the_msg )
/// Convenience macro for filtering and logging at the debug3 level
#define ARLOG_DEBUG3(the_msg) ARLOG_IF_LEVEL( olib::opencorelib::log_level::debug3, the_msg )
/// Convenience macro for filtering and logging at the debug4 level
#define ARLOG_DEBUG4(the_msg) ARLOG_IF_LEVEL( olib::opencorelib::log_level::debug4, the_msg )

#define ARLOG_DEBUG5(the_msg) ARLOG_IF_LEVEL( olib::opencorelib::log_level::debug5, the_msg )
#define ARLOG_DEBUG6(the_msg) ARLOG_IF_LEVEL( olib::opencorelib::log_level::debug6, the_msg )
#define ARLOG_DEBUG7(the_msg) ARLOG_IF_LEVEL( olib::opencorelib::log_level::debug7, the_msg )
#define ARLOG_DEBUG8(the_msg) ARLOG_IF_LEVEL( olib::opencorelib::log_level::debug8, the_msg )

/// Convenience conditional to check log filtering for the emergency level
#define ARCOND_EMERGENCY() ARCOND_LEVEL( olib::opencorelib::log_level::emergency )
/// Convenience conditional to check log filtering for the alert level
#define ARCOND_ALERT() ARCOND_LEVEL( olib::opencorelib::log_level::alert )
/// Convenience conditional to check log filtering for the critical level
#define ARCOND_CRIT() ARCOND_LEVEL( olib::opencorelib::log_level::critical )
/// Convenience conditional to check log filtering for the error level
#define ARCOND_ERR() ARCOND_LEVEL( olib::opencorelib::log_level::error )
/// Convenience conditional to check log filtering for the warning level
#define ARCOND_WARN() ARCOND_LEVEL( olib::opencorelib::log_level::warning )
/// Convenience conditional to check log filtering for the notice level
#define ARCOND_NOTICE() ARCOND_LEVEL( olib::opencorelib::log_level::notice )
/// Convenience conditional to check log filtering for the info level
#define ARCOND_INFO() ARCOND_LEVEL( olib::opencorelib::log_level::info )
/// Convenience conditional to check log filtering for the debug1 level
#define ARCOND_DEBUG() ARCOND_LEVEL( olib::opencorelib::log_level::debug1 )
/// Convenience conditional to check log filtering for the debug2 level
#define ARCOND_DEBUG2() ARCOND_LEVEL( olib::opencorelib::log_level::debug2 )
/// Convenience conditional to check log filtering for the debug3 level
#define ARCOND_DEBUG3() ARCOND_LEVEL( olib::opencorelib::log_level::debug3 )
/// Convenience conditional to check log filtering for the debug4 level
#define ARCOND_DEBUG4() ARCOND_LEVEL( olib::opencorelib::log_level::debug4 )
#define ARCOND_DEBUG5() ARCOND_LEVEL( olib::opencorelib::log_level::debug5 )
#define ARCOND_DEBUG6() ARCOND_LEVEL( olib::opencorelib::log_level::debug6 )
#define ARCOND_DEBUG7() ARCOND_LEVEL( olib::opencorelib::log_level::debug7 )
#define ARCOND_DEBUG8() ARCOND_LEVEL( olib::opencorelib::log_level::debug8 )

#define CREATE_LOGGER( expr, the_msg ) \
	if( !(expr) ); \
	else ++olib::opencorelib::logger::make_logger() \
                 .add_context(__FILE__, __LINE__, OLIB_CURRENT_FUNC_NAME, #expr ).msg((the_msg)).ARLOG_A

/** @def ARLOG_IF
	Works just as ARLOG, except that an expression is evaluated
    before the log request is sent to all log_targets. If the 
    expression is evaluated to true, the request goes through.
    If its false, no additional processing occurs (no messages
    are build), which makes this a cheap call in normal 
    circumstances.

	@sa log_handler
	@sa log_target
    @sa default_log_target
	@sa ARLOG
	@author Mats Lindel&ouml;f*/
#define ARLOG_IF(expr, the_msg ) CREATE_LOGGER( expr, the_msg )

/** @def ARCOND_LEVEL
    Conditional: evaluate to true if the argument log_level::serverity is
    lower than or equal to the log_handler::get_global_log_level()
    @author Rasmus Kaj */
#define ARCOND_LEVEL(test_level) \
        ((test_level) <= olib::opencorelib::the_log_handler::instance().get_global_log_level())

/** @def ARLOG_IF_LEVEL 
    This macro is similar to ARLOG_IF, except that 
    a log_level::severity is passed as the first argument. 
    This argument is compared to the CLog_handler::get_global_log_level()
    and if the passed value is lower than or equal to this 
    level, the logging takes place. If the value is higher no
    additional overhead is added.

    The disadvantage of this macro is that log_targets that
    cache log requests of higher log levels (used to output
    detailed crash information if a crash occurs) won't 
    be able to that. This must be considered but if speed
    is crucial, this macro is the one to opt for.

    @author Mats Lindel&ouml;f*/
#define ARLOG_IF_LEVEL( level_to_test_against_global , the_msg )       \
        if( !ARCOND_LEVEL(level_to_test_against_global) ){;}           \
        else ++olib::opencorelib::logger::make_logger()                \
             .add_context(__FILE__, __LINE__, OLIB_CURRENT_FUNC_NAME)  \
             .msg(the_msg).level(level_to_test_against_global).ARLOG_A

#define ARLOG_A(x) ARLOG_OP(x, B)
#define ARLOG_B(x) ARLOG_OP(x, A)

#define ARLOG_OP(x, next) \
	ARLOG_A.add_value(#x, (x)).ARLOG_ ## next \

/**/

#include "./logger.hpp"
#include "./loghandler.hpp"

#endif // _CORE_LOG_DEFINES_H_

