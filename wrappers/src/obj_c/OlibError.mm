
#define id _id_avoid_cpp_conflicts
#undef Nil

#include <boost/shared_ptr.hpp>
#include "opencorelib/cl/core.hpp"
#include "opencorelib/cl/minimal_string_defines.hpp"
#include "opencorelib/cl/base_exception.hpp"
#include "opencorelib/cl/exception_context.hpp"
#include "opencorelib/cl/str_util.hpp"

#undef id

#include "CoreConverts.h"

#import "OlibError.h"
#import "OlibErrorPrivate.h"

using namespace olib::opencorelib;

@implementation OlibError

- (id)init
{
    if( (self = [super init]) ) {
        _holder = new ExceptionHolder;
        exception_context ec("Unknown", 0, "Unknown", "Unknown", "Unknown");
        _holder->_obj = 
            boost::shared_ptr< base_exception >( 
                new base_exception( ec ) );
    }
    return self;
}

- (id)initWithStdException:(const std::exception&)aException
{
    if( (self = [self init]) ) {
        _holder->_obj = boost::shared_ptr< std::exception >( new std::exception(aException) );
    }
    return self;
}

- (id)initWithBaseException:(const base_exception&)aException
{
    if( (self = [self init]) ) {
        _holder->_obj = boost::shared_ptr< base_exception >( new base_exception(aException) );
    }
    return self;
}

- (void)dealloc
{
    delete _holder;
    [super dealloc];
}


- (void)finalize
{
    delete _holder;
    [super finalize];
}


- (NSString *)description
{
    return ConvertString( str_util::to_string(_holder->_obj->what()) );
}


- (NSString *)errorCode
{
    boost::shared_ptr< base_exception > temp = boost::dynamic_pointer_cast< base_exception >( _holder->_obj );
    if( temp ) {
        return ConvertString( temp->context().error_reason());
    }
    return nil;
}

- (NSString *)backTrace
{
    boost::shared_ptr< base_exception > temp = boost::dynamic_pointer_cast< base_exception >( _holder->_obj );
    if( temp ) {
        return ConvertString( temp->context().call_stack());
    }
    return nil;
}

- (NSString *)fileName
{
    boost::shared_ptr< base_exception > temp = boost::dynamic_pointer_cast< base_exception >( _holder->_obj );
    if( temp ) {
        return ConvertString( temp->context().filename());
    }
    return nil;
}

- (NSString *)function
{
    boost::shared_ptr< base_exception > temp = boost::dynamic_pointer_cast< base_exception >( _holder->_obj );
    if( temp ) {
        return ConvertString( temp->context().source());
    }
    return nil;
}

- (long)lineNumber
{
    boost::shared_ptr< base_exception > temp = boost::dynamic_pointer_cast< base_exception >( _holder->_obj );
    if( temp ) {
        return temp->context().line_nr();
    }
    return 0;
}


@end
