
// Copyright (C) 2009 Vizrt
// Released under the LGPL.

#ifndef OPENMEDIALIB_SCOPE_HANDLER_H_
#define OPENMEDIALIB_SCOPE_HANDLER_H_

#define LOKI_CLASS_LEVEL_THREADING
#include <loki/Singleton.h>
#include <openmedialib/ml/frame.hpp>
#include <list>
#include <map>
#include <boost/thread.hpp>
#include <boost/weak_ptr.hpp>

namespace olib { namespace openmedialib { namespace ml {

class ML_DECLSPEC lru_cache_type : public boost::noncopyable
{
public:
    
    lru_cache_type( )
    {}
    virtual ~lru_cache_type( )
    {}
    
    typedef std::pair< boost::int32_t, olib::openpluginlib::wstring > key_type;
    
    frame_type_ptr frame_for_position( const key_type &pos );
    void insert_frame_for_position( const key_type &pos, const frame_type_ptr& f );
    audio_type_ptr audio_for_position( const key_type &pos );
    void insert_audio_for_position( const key_type &pos, const audio_type_ptr& f );
    openimagelib::il::image_type_ptr image_for_position( const key_type &pos );
    void insert_image_for_position( const key_type &pos, const openimagelib::il::image_type_ptr& f );

	void clear( )
	{
		frames_.clear( );
		images_.clear( );
		audios_.clear( );
	}

private:
    
    void used( const key_type & pos );
    
    template< typename T >
    void insert_resource( const key_type &pos, const T& resource, std::map< key_type, T > &resource_map )
    {
        boost::mutex::scoped_lock lck( mutex_ );
        
        used( pos );
        resource_map[pos] = resource;
    }
    
    template< typename T >
    T get_resource( const key_type & pos, const std::map< key_type, T > &resource_map  )
    {
        boost::mutex::scoped_lock lck( mutex_ );
        
        typename std::map< key_type, T >::const_iterator map_it = resource_map.find( pos );
            
        if( map_it == resource_map.end() )
        {
            return T();
        }
        else
        {
            used( pos );
            return map_it->second;
        }
    }
    
    
    std::map< key_type, ml::frame_type_ptr > frames_;
    std::map< key_type, openimagelib::il::image_type_ptr > images_;
    std::map< key_type, ml::audio_type_ptr > audios_;
    
    std::list< key_type > lru_;
    
    boost::mutex mutex_;
    
};

typedef ML_DECLSPEC boost::shared_ptr< lru_cache_type > lru_cache_type_ptr;
typedef ML_DECLSPEC boost::weak_ptr< lru_cache_type > weak_lru_cache_type_ptr;

class scope_handler;

typedef ML_DECLSPEC Loki::SingletonHolder< scope_handler, Loki::CreateUsingNew, Loki::DefaultLifetime,
                               Loki::ClassLevelLockable > the_scope_handler;
    
class ML_DECLSPEC scope_handler : public boost::noncopyable
{
public:
    virtual ~scope_handler( )
    {}
    
    lru_cache_type_ptr lru_cache( const std::wstring &scope );
    
    template <typename T> friend struct Loki::CreateUsingNew;
    
private:
    scope_handler()
    {}
    
    std::map< std::wstring, weak_lru_cache_type_ptr > lru_cache_;
};

    
} /* ml */
} /* openmedialib */
} /* olib */



#endif
