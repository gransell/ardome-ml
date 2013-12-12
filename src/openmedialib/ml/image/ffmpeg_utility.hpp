#ifndef FFMPEG_UTILITY_H_
#define FFMPEG_UTILITY_H_

#include <openmedialib/ml/types.hpp>
#include <openmedialib/ml/image/image_types.hpp>

namespace olib { namespace openmedialib { namespace ml { namespace image {

struct UtilityAVPicture;
struct UtilityAVPixFmtDescriptor;

UtilityAVPixFmtDescriptor *utility_av_pix_fmt_desc_get( int pixfmt );
int utility_avpicture_get_size( int pixfmt, int width, int height );
void utility_av_pix_fmt_get_chroma_sub_sample( int pixfmt, int *chroma_w, int *chroma_h );
int utility_av_image_get_linesize( int pixfmt, int width, int plane ); 
void *utility_av_malloc( int bytes ); 
void utility_av_free( void *buf ); 
int utility_plane_count( int pixfmt );
int utility_bitdepth( int pixfmt, int index );
int utility_nb_components( int pixfmt );
int utility_av_image_alloc( boost::uint8_t *pointers[4], int linesizes[4], int w, int h, int pix_fmt, int align );
int utility_offset( int pixfmt, int index );
int utility_alpha( int pixfmt, int index );

bool is_pixfmt_rgb( int pixfmt );
bool is_pixfmt_planar( int pixfmt );
bool pixfmt_has_alpha( int pixfmt );

ML_DECLSPEC int ML_to_AV( MLPixelFormat pixfmt );
ML_DECLSPEC MLPixelFormat AV_to_ML( int pixfmt );

int rescale_and_convert_ffmpeg_image( ml::rescale_object_ptr ro, ml::image_type_ptr src, ml::image_type_ptr dst, int flags, bool safe = false );

} } } }

#endif
