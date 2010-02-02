

#import "OlibError.h"

#include <exception>

#include "boost/shared_ptr.hpp"

struct ExceptionHolder
{
    boost::shared_ptr< std::exception > _obj;
};

@interface OlibError (Private)

- (id)initWithStdException:(const std::exception&)aException;

- (id)initWithBaseException:(const olib::opencorelib::base_exception&)aException;

@end
