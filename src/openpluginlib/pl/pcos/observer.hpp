
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef PCOS_OBSERVER_H
#define PCOS_OBSERVER_H

#include <openpluginlib/pl/config.hpp>

namespace olib { namespace openpluginlib { namespace pcos {

class isubject;

/// Interface for observer
class OPENPLUGINLIB_DECLSPEC observer
{
public:
    virtual ~observer() {}

    virtual void updated( isubject* ) = 0;
};

} } }

#endif // PCOS_OBSERVER_H
