
// Copyright (C) 2009 Vizrt
// Released under the LGPL.

#ifndef OPENMEDIALIB_SCOPE_HANDLER_H_
#define OPENMEDIALIB_SCOPE_HANDLER_H_ value

#define LOKI_CLASS_LEVEL_THREADING
#include <loki/Singleton.h>
#include <frame.hpp>

namespace olib { namespace openmedialib { namespace ml {
    
class lru_cache_type
{
public:
    
    lru_cache_type( )
    {}
    virtual ~lru_cache_type( )
    {}
    
    const frame_type_ptr& frame_for_position( boost::int32_t pos );
    void push_frame_for_position( boost::int32_t pos, const frame_type& f );
    const audio_type_ptr& audio_for_position( boost::int32_t pos );
    void push_audio_for_position( boost::int32_t pos, const frame_type& f );
    const openimagelib::il::image_type_ptr& image_for_position( boost::int32_t pos );
    void push_image_for_position( boost::int32_t pos, const frame_type& f );

private:
    std::map< int, ml::frame_type_ptr > frames_;
    std::map< int, openimagelib::il::image_type_ptr > images_;
    std::map< int, ml::audio_type_ptr > audios_;
    
};

class scope_handler;

typedef Loki::SingletonHolder< scope_handler, Loki::CreateUsingNew, Loki::DefaultLifetime,
                               Loki::ClassLevelLockable > the_scope_handler;
    
class scope_handler
{
public:
    virtual ~scope_handler( )
    {}
    
    lru_cache_type& create_lru_cache( const std::string &scope );
    lru_cache_type& lru_cache( const std::string &scope );
    
    friend class Loki::SingletonHolder< scope_handler, Loki::CreateUsingNew, 
                                        Loki::DefaultLifetime, Loki::ClassLevelLockable >;
    
private:
    scope_handler()
    {}
    
    std::map< std::string, lru_cache_type > lru_cache_;
};

    
} /* ml */
} /* openmedialib */
} /* olib */



#endif
