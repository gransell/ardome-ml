#ifndef _CORE_COLOR_H_
#define _CORE_COLOR_H_

#include <vector>
#include "worker.hpp"

namespace olib
{
   namespace opencorelib
    {
        /// Represents a rgba color.
        /** This class simply holds four 1 byte values
            representing a color the most common way
            in programming. amf's graph library normally
            represent colors in yuv444. 
            @author Mats Lindel&ouml;f*/
        class CORE_API rgba_color
        {
        public:
            /// Create a new color instance.
            /** @param r The red component
                @param g The green component
                @param b The blue component
                @param a The alpha, mask or transparency component. */
            rgba_color( boost::uint8_t r, 
                        boost::uint8_t g, 
                        boost::uint8_t b, 
                        boost::uint8_t a = 255 );

            /// Create a color from a byte array.
            /** The array must be at least four bytes long.
                @param comps A pointer to a four (or longer) element byte array. */
            rgba_color( boost::uint8_t* comps );

            /// Copy a color into a new one.
            rgba_color( const rgba_color& other );

            /// Assign a color to an existing instance.
            rgba_color& operator=( const rgba_color& other ); 

            /// Get the red component.
            boost::uint8_t get_r() const { return m_components[0]; }

            /// Set the red component.
            void set_r( boost::uint8_t r ) { m_components[0] = r ; }
            
            /// Get the green component.
            boost::uint8_t get_g() const { return m_components[1]; }

            /// Set the green component.
            void set_g( boost::uint8_t g ) { m_components[1] = g ; }
            
            /// Get the blue component.
            boost::uint8_t get_b() const { return m_components[2]; }

            /// Set the blue component.
            void set_b( boost::uint8_t b ) { m_components[2] = b ; }

            /// Get the alpha component.
            /** A value of 0 means that the pixel is totally transparent, 
                255 that it is fully opaque. */
            boost::uint8_t get_alpha() const { return m_components[3]; }

            /// Set the alpha component.
            void set_alpha( boost::uint8_t a ) { m_components[3] = a ; }

            /// Get the color as a four byte array, [r,g,b,a].
            const boost::uint8_t* get_components() const { return m_components; }

            /// Convert the color to a string
            /** Example: (255,0,0,255), non-transparent red. */
            t_string to_string() const;

			CORE_API friend t_ostream& operator<<( t_ostream& os, const rgba_color& col);

            /// Get one of the default colors by using a string.
            /** @param col_name A valid color name. 
                        The static values in this class are valid names */
            static rgba_color default_color(const t_string& col_name);

            static rgba_color aliceblue();
            static rgba_color antiquewhite();
            static rgba_color aqua();
            static rgba_color aquamarine();
            static rgba_color azure();
            static rgba_color beige();
            static rgba_color bisque();
            static rgba_color black();
            static rgba_color blanchedalmond();
            static rgba_color blue();
            static rgba_color blueviolet();
            static rgba_color brown();
            static rgba_color burlywood();
            static rgba_color cadetblue();
            static rgba_color chartreuse();
            static rgba_color chocolate();
            static rgba_color coral();
            static rgba_color cornflowerblue();
            static rgba_color cornsilk();
            static rgba_color crimson();
            static rgba_color cyan();
            static rgba_color darkblue();
            static rgba_color darkcyan();
            static rgba_color darkgoldenrod();
            static rgba_color darkgray();
            static rgba_color darkgreen();
            static rgba_color darkkhaki();
            static rgba_color darkmagenta();
            static rgba_color darkolivegreen();
            static rgba_color darkorange();
            static rgba_color darkorchid();
            static rgba_color darkred();
            static rgba_color darksalmon();
            static rgba_color darkseagreen();
            static rgba_color darkslateblue();
            static rgba_color darkslategray();
            static rgba_color darkslategrey();
            static rgba_color darkturquoise();
            static rgba_color darkviolet();
            static rgba_color deeppink();
            static rgba_color deepskyblue();
            static rgba_color dimgray();
            static rgba_color dimgrey();
            static rgba_color dodgerblue();
            static rgba_color firebrick();
            static rgba_color floralwhite();
            static rgba_color forestgreen();
            static rgba_color fuchsia();
            static rgba_color gainsboro();
            static rgba_color ghostwhite();
            static rgba_color gold();
            static rgba_color goldenrod();
            static rgba_color gray();
            static rgba_color grey();
            static rgba_color green();
            static rgba_color greenyellow();
            static rgba_color honeydew();
            static rgba_color hotpink();
            static rgba_color indianred();
            static rgba_color indigo();
            static rgba_color ivory();
            static rgba_color khaki();
            static rgba_color lavender();
            static rgba_color lavenderblush();
            static rgba_color lawngreen();
            static rgba_color lemonchiffon();
            static rgba_color lightblue();
            static rgba_color lightcoral();
            static rgba_color lightcyan();
            static rgba_color lightgoldenrodyellow();
            static rgba_color lightgray();
            static rgba_color lightgreen();
            static rgba_color lightgrey();
            static rgba_color lightpink();
            static rgba_color lightsalmon();
            static rgba_color lightseagreen();
            static rgba_color lightskyblue();
            static rgba_color lightslategray();
            static rgba_color lightslategrey();
            static rgba_color lightsteelblue();
            static rgba_color lightyellow();
            static rgba_color lime();
            static rgba_color limegreen();
            static rgba_color linen();
            static rgba_color magenta();
            static rgba_color maroon();
            static rgba_color mediumaquamarine();
            static rgba_color mediumblue();
            static rgba_color mediumorchid();
            static rgba_color mediumpurple();
            static rgba_color mediumseagreen();
            static rgba_color mediumslateblue();
            static rgba_color mediumspringgreen();
            static rgba_color mediumturquoise();
            static rgba_color mediumvioletred();
            static rgba_color midnightblue();
            static rgba_color mintcream();
            static rgba_color mistyrose();
            static rgba_color moccasin();
            static rgba_color navajowhite();
            static rgba_color navy();
            static rgba_color oldlace();
            static rgba_color olive();
            static rgba_color olivedrab();
            static rgba_color orange();
            static rgba_color orangered();
            static rgba_color orchid();
            static rgba_color palegoldenrod();
            static rgba_color palegreen();
            static rgba_color paleturquoise();
            static rgba_color palevioletred();
            static rgba_color papayawhip();
            static rgba_color peachpuff();
            static rgba_color peru();
            static rgba_color pink();
            static rgba_color plum();
            static rgba_color powderblue();
            static rgba_color purple();
            static rgba_color red();
            static rgba_color rosybrown();
            static rgba_color royalblue();
            static rgba_color saddlebrown();
            static rgba_color salmon();
            static rgba_color sandybrown();
            static rgba_color seagreen();
            static rgba_color seashell();
            static rgba_color sienna();
            static rgba_color silver();
            static rgba_color skyblue();
            static rgba_color slateblue();
            static rgba_color slategray();
            static rgba_color slategrey();
            static rgba_color snow();
            static rgba_color springgreen();
            static rgba_color steelblue();
            static rgba_color tan();
            static rgba_color teal();
            static rgba_color thistle();
            static rgba_color tomato();
            static rgba_color turquoise();
            static rgba_color violet();
            static rgba_color wheat();
            static rgba_color white();
            static rgba_color whitesmoke();
            static rgba_color yellow();
            static rgba_color yellowgreen();

        private:
            boost::uint8_t m_components[4];

            // Stores the default color values
            static std::map< t_string, rgba_color > m_default_colors;
            static 	const std::map< t_string, rgba_color >&  get_default_colors();
            static void create_default_colors();
        };

		CORE_API t_ostream& operator<<( t_ostream& os, const rgba_color& col);
    }
}

#endif // _CORE_COLOR_H_

