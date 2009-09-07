#ifndef AVFORMAT_INDEX_
#define AVFORMAT_INDEX_

#include <boost/cstdint.hpp>

namespace olib { namespace openmedialib { namespace ml { 

class aml_index
{
	public:
		// Virtual destructor
		virtual ~aml_index( ) { }

		/// Read any pending data in the index
		virtual void read( ) = 0;

		/// Find the byte position for the position
		virtual boost::int64_t find( int position ) = 0;

		/// Return the number of frames as reported by the index
		/// Note that if we have not reached the eof, there is a chance that the index won't
		/// accurately reflect the number of frames in the available media (since we can't
		/// guarantee that the two files will be in sync), so prior to the reception of the 
		/// terminating record, we need to return an approximation which is > current
		virtual int frames( int current ) = 0;

		/// Indicates if the index is complete
		virtual bool finished( ) = 0;

		/// Returns the size of the data file as indicated by the index
		virtual boost::int64_t bytes( ) = 0;

		/// Calculate the number of available frames for the given media data size 
		/// Note that the size given can be less than, equal to or greater than the size
		/// which the index reports - any data greater than the index should be ignored, 
		/// and data less than the index should report a frame count such that the last
		/// gop can be full decoded
		virtual int calculate( boost::int64_t size ) = 0;

		/// Indicates if the media can use the index for byte accurate seeking or not
		/// This is a work around for audio files which don't like byte positions
		virtual bool usable( ) = 0;

		/// User of the index can indicate byte seeking usability here
		virtual void set_usable( bool value ) = 0;

		// Return the key frame associated to the position
		virtual int key_frame_of( int position ) = 0;

		// Return the key frame associated to the offset
		virtual int key_frame_from( boost::int64_t offset ) = 0;
};

extern aml_index *aml_index_factory( const char * );

} } }

#endif

