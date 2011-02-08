#ifndef _CORE_MEDIA_DEFINITIONS_H_
#define _CORE_MEDIA_DEFINITIONS_H_

/** @file media_definitions.h
    Different definitions related to media. */

//#include "./core.hpp"
#include "./typedefs.hpp"

namespace olib
{
   namespace opencorelib
    {
        /// Encapsulates enums and functions related to frame rate issues.
        namespace frame_rate
        {
            enum type 
            { 
                pal, /**<   short for phase-alternating line, colour 
                            encoding system used in broadcast television 
                            systems in large parts of the world. In digital video 
                            applications, such as DVDs and digital broadcasting, 
                            colour encoding is no longer significant; in that context, 
                            PAL means only: 
                            <ul>
                                <li>576 lines 
                                <li>25 frames/50 fields {second} 
                                <li>interlaced video 
                                <li>PCM audio (baseband) 
                            </ul>*/
                ntsc, /**<  The NTSC standard of color television:
                            525 scan lines interlaced with a 60*1000/1001 Hz refresh rate
                            525/2 = 262.5 lines/field
                            NTSC refresh rate is not 59.95 Hz exactly but 60 Hz*1000/1001 
                            = 59.9400599400599400... Hz ò 59.94 Hz. */ 

                movie, /**< 24000:1001.
   
                internal = pal,  The internal format is pal. */
                undef 
            };

            /// Get the frame rate per second for a certain frame_rate::type
            CORE_API rational_time get_fps( olib::opencorelib::frame_rate::type ft );

            /// Convert to a frame_rate::type from a fps and drop frame flag.
            CORE_API olib::opencorelib::frame_rate::type get_type( const rational_time& rt );

            /// Convert the enum to a human readable string.
            CORE_API const TCHAR* to_string( type ft );

			CORE_API type to_type( const t_string& str );

        }

        /// Encapsulates the scanmode type. 
        namespace scanmode
        {
            enum type 
            { 
                interlaced,  /**< The image is interlaced. Interlaced means that every second
                                    line of pixels in the image is carried by each frame. To
                                    make a complete image two consecutive images (often called
                                    fields) are needed. This is the common case on television sets.*/
                progressive  /**< The image is progressive. This means that all lines of pixels
                                    are carried in each frame. This is the common case on 
                                    computer screens.*/
            };
        }


        /// Carries information about the current display.
        class CORE_API display_info
        {
        public:

            /// Creates a new display_info object.
            /** @param x The x resolution of the display.
                @param y The y resolution of the display.
                @param st The scanmode of the display. */
            display_info(int x, int y, olib::opencorelib::scanmode::type st ): m_x(x), m_y(y), m_scanmode(st) {}
            
            /** 720p means that the resolution of the picture is 1280 vertical 
                pixels by 720 horizontal pixels progressive. progressive 
                scanning offers a smoother picture as 720 horizontal 
                lines are scanned progressively or in succession in a 
                vertical frame that is repeated, in the USA, 30 times a second*/
            static display_info HD_720p() { return display_info(1280, 720, scanmode::progressive ); }  

            /** 1080p means that the resolution of the picture is 1920 vertical 
                pixels by 1080 horizontal pixels and p stands again for 
                progressive scanning. This format works on the same principle 
                as 720p; the only difference is that in this type there are 
                more pixels and the resolution is better */
            static display_info HD_1080p() { return display_info(1920, 1080, scanmode::progressive); }

            /** 1080i means that the resolution of the picture is 1920 
                vertical pixels by 1080 horizontal pixels interlaced */
            static display_info HD_1080i() { return display_info(1920, 1080, scanmode::interlaced ); }  

            /// Get the x resolution of the screen.
            int get_x() const { return m_x;}

            /// Set the x resolution of the screen.
            void set_x(int a) { m_x = a; }

            /// Get the y resolution of the screen.
            int get_y() const { return m_y;}

            /// Set the y resolution of the screen.
            void set_y(int a) { m_y = a; }

            /// Get the scanmode.
            olib::opencorelib::scanmode::type get_scanmode() const { return m_scanmode; }

            /// Set the scanmode.
            void set_scanmode( olib::opencorelib::scanmode::type st ) { m_scanmode = st; }
        private:
            int m_x, m_y;
            scanmode::type m_scanmode;
        };

        /// Encapsulates different video file formats.
        namespace video_file_format
        {
            enum type
            {
                dvdif,      /**< dvdif format */
                t_mpeg1,    /**< mpeg1 */
                mpeg2,      /**< mpeg2 */
                dvc_pro,    /**< dvc_pro */
                mjpeg,      /**< mjpeg */
                jpeg_2000   /**< jpeg_2000 */
            };
        }

        /// Different flags related to video.
        namespace video_flag
        {
            enum flags
            {
                no_flags,
                imx,
                vbr
            };
        }

        /// The aspect ration of the video.
        namespace video
        {
            enum aspect
            {
                aspect4_3,
                aspect16_9 /**< The aspect ratio of widescreen DTV formats used in 
                                all HDTV (High Definition TV) and some SDTV 
                                (Standard Definition TV); it stands for 16 arbitrary 
                                units of width for every 9 arbitrary units of height. */
            };
        }

        /// Different audio file formats.
        namespace audio_file_format
        {
            enum type
            {
                pcm, /**< Pulse code modulation, pure, decoded audio. */
                mp2, /**< mp2 encoded audio. */
                mp3, /**< mp3 encoded audio. */
                aes3,/**< aes3 encoded audio. */
                ac3  /**< ac3 encoded audio */
            };
        }
    }
}


#endif //_CORE_MEDIA_DEFINITIONS_H_

