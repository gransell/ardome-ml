
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <map>
#include <algorithm>
#include <iostream>

#include <openpluginlib/pl/pcos/subject.hpp>
#include <openpluginlib/pl/pcos/observer.hpp>

namespace olib { namespace openpluginlib { namespace pcos {

class subject::subject_impl
{
public:
    subject_impl()
        {}

    /// a map of observer, lock count
    typedef std::map< boost::shared_ptr< observer >, unsigned int > observer_container;
    observer_container observers;
};

subject::subject()
    : impl_( new subject_impl() )
{
    // std::cout << "subject::CTOR: " << this << "\n";
}

subject::~subject()
{
    // std::cout << "subject::DTOR: " << this << "\n";
}

void subject::attach( boost::shared_ptr< observer > obs )
{
    // std::cout << "subject::attach: " << obs.get() << "\n";
    impl_->observers.insert( std::make_pair( obs, 0 ) );
}

void subject::detach( boost::shared_ptr< observer > obs )
{
    subject_impl::observer_container::iterator I = impl_->observers.find( obs );
    if ( I == impl_->observers.end() )
    {
        return;
    }

    impl_->observers.erase( I );
}

template < typename T > struct Notify
{
    Notify( subject* s )
        : m_subject( s )
        {}

    typedef typename T::value_type value_type;

    void operator()( const value_type& v )
    {
        if ( v.second )
        {
            // we have a non-zero block count
            // std::cout << "blocked observer found " << v.first.get() << "\n";
            return;
        }

        // std::cout << "updating observer " << v.first.get() << "\n";
        v.first->updated( dynamic_cast<isubject*>(m_subject) );
    }

    subject* m_subject;
};

void subject::update()
{
    std::for_each( impl_->observers.begin(), impl_->observers.end(), Notify< subject_impl::observer_container >( this ) );
}

void subject::block( boost::shared_ptr< observer > wp )
{
    if ( !impl_->observers.count( wp ) )
    {
        return;
    }

    impl_->observers[ wp ]++;
}

void subject::unblock( boost::shared_ptr< observer > wp )
{
    if ( !impl_->observers.count( wp ) )
    {
        return;
    }

    impl_->observers[ wp ]--;
}


} } }
