
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef PCOS_ISUBJECT_H
#define PCOS_ISUBJECT_H

#include <openpluginlib/pl/config.hpp>

// boost
#include <boost/shared_ptr.hpp>

namespace olib { namespace openpluginlib { namespace pcos {

class observer;

/// A standard subject to be part of the observer pattern.
/// The observer pointers are held by shared references.
class OPENPLUGINLIB_DECLSPEC isubject
{
public:
    virtual ~isubject() {}

    //// attach an observer to this subject
    virtual void attach( boost::shared_ptr< observer > ) = 0;

    /// detach an observer from this subject
    virtual void detach( boost::shared_ptr< observer > ) = 0;

    /// notify all observers
    virtual void update() = 0;

    /// block notification for an observer; this is 
    /// a counted block operation i.e. if you call
    /// block n times on the same observer, no 
    /// notification will be given until unblock()
    /// has been called a matching number of times.
    virtual void block( boost::shared_ptr< observer > ) = 0;

    /// unblock notification for an observer
    virtual void unblock( boost::shared_ptr< observer > ) = 0;
};

} } }

#endif // PCOS_ISUBJECT_H
