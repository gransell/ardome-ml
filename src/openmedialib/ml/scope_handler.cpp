
#include "scope_handler.hpp"

namespace olib { namespace openmedialib { namespace ml {
    
    lru_cache_type& scope_handler::create_lru_cache( const std::string &scope )
    {
        std::map< std::string, lru_cache_type >::iterator cache_it = lru_cache_.find( scope );
        ARENFORCE_MSG( cache_it == lru_cache_.end(), _CT("An LRU cache already exists for the scope %1%") )( scope );
        
        return lru_cache_[scope];
    }
    
    lru_cache_type& scope_handler::lru_cache( const std::string &scope );
    {
        std::map< std::string, lru_cache_type >::iterator cache_it = lru_cache_.find( scope );
        ARENFORCE_MSG( cache_it != lru_cache_.end(), _CT("No LRU cache exists for the scope %1%") )( scope );
        
        return cache_it->second;
    }
    
} /* ml */
} /* openmedialib */
} /* olib */
