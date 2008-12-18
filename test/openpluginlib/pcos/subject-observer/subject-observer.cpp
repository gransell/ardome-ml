
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <cassert>
#include <iostream>

#include <openpluginlib/pl/pcos/subject.hpp>
#include <openpluginlib/pl/pcos/observer.hpp>

namespace opl = olib::openpluginlib;
namespace pcos = olib::openpluginlib::pcos;

class counting_observer : public pcos::observer
{
public:
    counting_observer()
        : count( 0 )
        {}

    virtual void updated( pcos::isubject* )
    {
        ++count;
    }

    unsigned int count;
};

int main( )
{
    // simple test
    {
        pcos::subject subj;
        boost::shared_ptr< counting_observer > obs( new counting_observer );
        subj.attach( obs );

        subj.update();
        assert( obs->count == 1 );
    }

    // detach test
    {
        pcos::subject subj;
        boost::shared_ptr< counting_observer > obs1( new counting_observer );
        boost::shared_ptr< counting_observer > obs2( new counting_observer );
        subj.attach( obs1 );
        subj.attach( obs2 );

        subj.update();
        assert( obs1->count == 1 );
        assert( obs2->count == 1 );

        subj.detach( obs2 );

        subj.update();
        assert( obs1->count == 2 );
        assert( obs2->count == 1 );

    }

    // multiple observers
    {
        pcos::subject subj;
        boost::shared_ptr< counting_observer > obs1( new counting_observer );
        boost::shared_ptr< counting_observer > obs2( new counting_observer );
        subj.attach( obs1 );
        subj.attach( obs2 );

        subj.update();
        assert( obs1->count == 1 );
        assert( obs2->count == 1 );
    }

    // multiple subjects
    {
        pcos::subject subj1;
        pcos::subject subj2;
        boost::shared_ptr< counting_observer > obs( new counting_observer );
        subj1.attach( obs );
        subj2.attach( obs );

        subj1.update();
        subj2.update();
        assert( obs->count == 2 );
    }

    // stale pointer
    {
        pcos::subject subj;
        {
            boost::shared_ptr< counting_observer > obs( new counting_observer );
            subj.attach( obs );
            
            subj.update();
            assert( obs->count == 1 );
        }
        subj.update();
    }

    // block notification
    {
        pcos::subject subj;
        boost::shared_ptr< counting_observer > obs1( new counting_observer );
        boost::shared_ptr< counting_observer > obs2( new counting_observer );
        subj.attach( obs1 );
        subj.attach( obs2 );

        subj.update();
        assert( obs1->count == 1 );
        assert( obs2->count == 1 );

        subj.block( obs1 );
        subj.update();
        assert( obs1->count == 1 );
        assert( obs2->count == 2 );

        subj.unblock( obs1 );
        subj.update();
        assert( obs1->count == 2 );
        assert( obs2->count == 3 );
    }

    // counted block
    {
        pcos::subject subj;
        boost::shared_ptr< counting_observer > obs( new counting_observer );
        subj.attach( obs );

        subj.update();
        assert( obs->count == 1 );

        subj.block( obs );
        subj.block( obs );

        subj.update();
        assert( obs->count == 1 );

        subj.unblock( obs );

        subj.update();
        assert( obs->count == 1 );

        subj.unblock( obs );

        subj.update();
        assert( obs->count == 2 );
    }


    return 0;
}
