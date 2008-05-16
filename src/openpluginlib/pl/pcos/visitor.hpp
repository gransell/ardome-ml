
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef PCOS_VISITOR_H
#define PCOS_VISITOR_H

#include <openpluginlib/pl/config.hpp>

namespace olib { namespace openpluginlib { namespace pcos {

class property;
class property_container;

class OPENPLUGINLIB_DECLSPEC visitor
{
public:
    virtual ~visitor() {};

    virtual bool visit_property( property& ) = 0;
    virtual bool visit_property_container( property_container& ) = 0;

};

} } }

#endif // PCOS_VISITOR_H
