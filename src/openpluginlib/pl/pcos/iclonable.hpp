
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.


#ifndef PCOS_ICLONABLE_H
#define PCOS_ICLONABLE_H

#include <openpluginlib/pl/config.hpp>

namespace olib { namespace openpluginlib { namespace pcos {

/// interface to be implemented by classes that want a polymorphic
/// copy constructor. As most modern compilers now support co-variant return types, 
/// upon sub-classing the return type can be modified to the more specific type.
class OPENPLUGINLIB_DECLSPEC iclonable
{
public:
    virtual ~iclonable() {};

    /// returns a new deep copy.
    virtual iclonable* clone() const = 0;
};

} } }

#endif // PCOS_ICLONABLE_H
