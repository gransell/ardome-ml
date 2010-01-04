
#include "scope_handler.hpp"

namespace il = olib::openimagelib::il;

using std::list;
using std::map;

namespace
{
    static const size_t LRU_MAX_SIZE = 32;
}


namespace olib { namespace openmedialib { namespace ml {
    
    lru_cache_type_ptr scope_handler::lru_cache( const std::wstring &scope )
    {
        if( lru_cache_.find( scope ) == lru_cache_.end() )
        {
            lru_cache_[scope] = lru_cache_type_ptr( new lru_cache_type() );
        }
        
        return lru_cache_[scope];
    }
    
    
    void lru_cache_type::used( const key_type &pos )
    {
        list< key_type >::iterator lru_it = std::find( lru_.begin(), lru_.end(), pos );
        
        if( lru_it == lru_.end() )
        {
            if( lru_.size() >= LRU_MAX_SIZE )
            {
                key_type k = lru_.back();
                lru_.pop_back();
                frames_.erase( k );
                audios_.erase( k );
                images_.erase( k );
            }
        }
        else
        {
            lru_.erase( lru_it );
        }
        
        lru_.push_front( pos );
    }
    
    frame_type_ptr lru_cache_type::frame_for_position( const key_type &pos )
    {
        return get_resource<frame_type_ptr>( pos, frames_ );
    }
    
    void lru_cache_type::insert_frame_for_position( const key_type &pos, const frame_type_ptr& f )
    {
        insert_resource( pos, f, frames_ );
    }
    
    audio_type_ptr lru_cache_type::audio_for_position( const key_type &pos )
    {
        return get_resource( pos, audios_ );
    }
    
    void lru_cache_type::insert_audio_for_position( const key_type &pos, const audio_type_ptr& a )
    {
        insert_resource( pos, a, audios_ );
    }
    
    openimagelib::il::image_type_ptr lru_cache_type::image_for_position( const key_type &pos )
    {
        return get_resource( pos, images_ );
    }
    
    void lru_cache_type::insert_image_for_position( const key_type &pos, const il::image_type_ptr& i )
    {
        insert_resource( pos, i, images_ );
    }
    
} /* ml */
} /* openmedialib */
} /* olib */
