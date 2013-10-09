#ifndef plugin_decode_filter_analyse_h
#define plugin_decode_filter_analyse_h

#include <openmedialib/ml/filter_simple.hpp>
#include <openmedialib/ml/analyse.hpp>

namespace ml = olib::openmedialib::ml;

namespace olib { namespace openmedialib { namespace ml { namespace decode {

class filter_analyse : public ml::filter_simple
{
public:
    // Filter_type overloads
    filter_analyse( );
    
    virtual ~filter_analyse( );
    
    // Indicates if the input will enforce a packet decode
    virtual bool requires_image( ) const;
    
    // This provides the name of the plugin (used in serialisation)
    virtual const std::wstring get_uri( ) const;
    
protected:

    void do_fetch( ml::frame_type_ptr &result );

    bool inner_fetch( ml::frame_type_ptr &result, int position, bool parse = true );

	std::map< int, ml::frame_type_ptr > gop_;
	ml::analyse_ptr analyse_;
};

} } } }

#endif

