#include "precompiled_headers.hpp"
#include "./color.hpp"

namespace olib
{
   namespace opencorelib
    {
        rgba_color::rgba_color( boost::uint8_t r, 
                                boost::uint8_t g, 
                                boost::uint8_t b, 
                                boost::uint8_t a )
        {
            m_components[0] = r;
            m_components[1] = g;
            m_components[2] = b;
            m_components[3] = a;
        }

        rgba_color::rgba_color( boost::uint8_t* comps)
        {
            std::copy(comps, comps + 4, m_components );
        }

        rgba_color::rgba_color( const rgba_color& other )
        {
            std::copy(other.m_components, other.m_components + 4, m_components );
        }

        rgba_color& rgba_color::operator=( const rgba_color& other )
        {
            std::copy(other.m_components, other.m_components + 4, m_components );
            return *this;
        }


        t_string rgba_color::to_string() const
        {
            t_format fmt(_CT("(%i,%i,%i,%i)"));
            return ( fmt % (int)m_components[0] % (int)m_components[1]
                            % (int)m_components[2] % (int)m_components[3] ).str();

        }

		CORE_API t_ostream& operator<<( t_ostream& os, const rgba_color& col)
		{
			os << _CT("<rgba_color>");
			os << col.to_string();
			os << _CT("</rgba_color>");
			return os;
		}
        
        std::map< t_string, rgba_color > rgba_color::m_default_colors;

        rgba_color rgba_color::aliceblue(){ return rgba_color(240, 248, 255); }
        rgba_color rgba_color::antiquewhite(){ return rgba_color(250, 235, 215); }
        rgba_color rgba_color::aqua(){ return rgba_color(0, 255, 255); }
        rgba_color rgba_color::aquamarine(){ return rgba_color(127, 255, 212); }
        rgba_color rgba_color::azure(){ return rgba_color(240, 255, 255); }
        rgba_color rgba_color::beige(){ return rgba_color(245, 245, 220); }
        rgba_color rgba_color::bisque(){ return rgba_color(255, 228, 196); }
        rgba_color rgba_color::black(){ return rgba_color(0, 0, 0); }
        rgba_color rgba_color::blanchedalmond(){ return rgba_color(255, 235, 205); }
        rgba_color rgba_color::blue(){ return rgba_color(0, 0, 255); }
        rgba_color rgba_color::blueviolet(){ return rgba_color(138, 43, 226); }
        rgba_color rgba_color::brown(){ return rgba_color(165, 42, 42); }
        rgba_color rgba_color::burlywood(){ return rgba_color(222, 184, 135); }
        rgba_color rgba_color::cadetblue(){ return rgba_color(95, 158, 160); }
        rgba_color rgba_color::chartreuse(){ return rgba_color(127, 255, 0); }
        rgba_color rgba_color::chocolate(){ return rgba_color(210, 105, 30); }
        rgba_color rgba_color::coral(){ return rgba_color(210, 105, 30); }
        rgba_color rgba_color::cornflowerblue(){ return rgba_color(100, 149, 237); }
        rgba_color rgba_color::cornsilk(){ return rgba_color(255, 248, 220); }
        rgba_color rgba_color::crimson(){ return rgba_color(220, 20, 60); }
        rgba_color rgba_color::cyan(){ return rgba_color(0, 255, 255); }
        rgba_color rgba_color::darkblue(){ return rgba_color(0, 0, 139); }
        rgba_color rgba_color::darkcyan(){ return rgba_color(0, 139, 139); }
        rgba_color rgba_color::darkgoldenrod(){ return rgba_color(184, 134, 11); }
        rgba_color rgba_color::darkgray(){ return rgba_color(169, 169, 169); }
        rgba_color rgba_color::darkgreen(){ return rgba_color(0, 100, 0); }
        rgba_color rgba_color::darkkhaki(){ return rgba_color(189, 183, 107); }
        rgba_color rgba_color::darkmagenta(){ return rgba_color(139, 0, 139); }
        rgba_color rgba_color::darkolivegreen(){ return rgba_color( 85, 107, 47); }
        rgba_color rgba_color::darkorange(){ return rgba_color(255, 140, 0); }
        rgba_color rgba_color::darkorchid(){ return rgba_color(153, 50, 204); }
        rgba_color rgba_color::darkred(){ return rgba_color(139, 0, 0); }
        rgba_color rgba_color::darksalmon(){ return rgba_color(233, 150, 122); }
        rgba_color rgba_color::darkseagreen(){ return rgba_color(143, 188, 143); }
        rgba_color rgba_color::darkslateblue(){ return rgba_color( 72, 61, 139); }
        rgba_color rgba_color::darkslategray(){ return rgba_color( 47, 79, 79); }
        rgba_color rgba_color::darkslategrey(){ return rgba_color( 47, 79, 79); }
        rgba_color rgba_color::darkturquoise(){ return rgba_color( 0, 206, 209); }
        rgba_color rgba_color::darkviolet(){ return rgba_color(148, 0, 211); }
        rgba_color rgba_color::deeppink(){ return rgba_color(255, 20, 147); }
        rgba_color rgba_color::deepskyblue(){ return rgba_color( 0, 191, 255); }
        rgba_color rgba_color::dimgray(){ return rgba_color(105, 105, 105); }
        rgba_color rgba_color::dimgrey(){ return rgba_color(105, 105, 105); }
        rgba_color rgba_color::dodgerblue(){ return rgba_color( 30, 144, 255); }
        rgba_color rgba_color::firebrick(){ return rgba_color(178, 34, 34); }
        rgba_color rgba_color::floralwhite(){ return rgba_color(255, 250, 240); }
        rgba_color rgba_color::forestgreen(){ return rgba_color( 34, 139, 34); }
        rgba_color rgba_color::fuchsia(){ return rgba_color(255, 0, 255); }
        rgba_color rgba_color::gainsboro(){ return rgba_color(220, 220, 220); }
        rgba_color rgba_color::ghostwhite(){ return rgba_color(248, 248, 255); }
        rgba_color rgba_color::gold(){ return rgba_color(255, 215, 0); }
        rgba_color rgba_color::goldenrod(){ return rgba_color(218, 165, 32); }
        rgba_color rgba_color::gray(){ return rgba_color(128, 128, 128); }
		rgba_color rgba_color::grey(){ return rgba_color(128, 128, 128); }
        rgba_color rgba_color::green(){ return rgba_color( 0, 128, 0); }
        rgba_color rgba_color::greenyellow(){ return rgba_color(173, 255, 47); }
        rgba_color rgba_color::honeydew(){ return rgba_color(240, 255, 240); }
        rgba_color rgba_color::hotpink(){ return rgba_color(255, 105, 180); }
        rgba_color rgba_color::indianred(){ return rgba_color(205, 92, 92); }
        rgba_color rgba_color::indigo(){ return rgba_color( 75, 0, 130); }
        rgba_color rgba_color::ivory(){ return rgba_color(255, 255, 240); }
        rgba_color rgba_color::khaki(){ return rgba_color(240, 230, 140); }
        rgba_color rgba_color::lavender(){ return rgba_color(230, 230, 250); }
        rgba_color rgba_color::lavenderblush(){ return rgba_color(255, 240, 245); }
        rgba_color rgba_color::lawngreen(){ return rgba_color(124, 252, 0); }
        rgba_color rgba_color::lemonchiffon(){ return rgba_color(255, 250, 205); }
        rgba_color rgba_color::lightblue(){ return rgba_color(173, 216, 230); }
        rgba_color rgba_color::lightcoral(){ return rgba_color(240, 128, 128); }
        rgba_color rgba_color::lightcyan(){ return rgba_color(224, 255, 255); }
        rgba_color rgba_color::lightgoldenrodyellow(){ return rgba_color(250, 250, 210); }
        rgba_color rgba_color::lightgray(){ return rgba_color(211, 211, 211); }
        rgba_color rgba_color::lightgreen(){ return rgba_color(144, 238, 144); }
        rgba_color rgba_color::lightgrey(){ return rgba_color(211, 211, 211); }
        rgba_color rgba_color::lightpink(){ return rgba_color(255, 182, 193); }
        rgba_color rgba_color::lightsalmon(){ return rgba_color(255, 160, 122); }
        rgba_color rgba_color::lightseagreen(){ return rgba_color( 32, 178, 170); }
        rgba_color rgba_color::lightskyblue(){ return rgba_color(135, 206, 250); }
        rgba_color rgba_color::lightslategray(){ return rgba_color(119, 136, 153); }
        rgba_color rgba_color::lightslategrey(){ return rgba_color(119, 136, 153); }
        rgba_color rgba_color::lightsteelblue(){ return rgba_color(176, 196, 222); }
        rgba_color rgba_color::lightyellow(){ return rgba_color(255, 255, 224); }
        rgba_color rgba_color::lime(){ return rgba_color( 0, 255, 0); }
        rgba_color rgba_color::limegreen(){ return rgba_color( 50, 205, 50); }
        rgba_color rgba_color::linen(){ return rgba_color(250, 240, 230); }
        rgba_color rgba_color::magenta(){ return rgba_color(255, 0, 255); }
        rgba_color rgba_color::maroon(){ return rgba_color(128, 0, 0); }
        rgba_color rgba_color::mediumaquamarine(){ return rgba_color(102, 205, 170); }
        rgba_color rgba_color::mediumblue(){ return rgba_color( 0, 0, 205); }
        rgba_color rgba_color::mediumorchid(){ return rgba_color(186, 85, 211); }
        rgba_color rgba_color::mediumpurple(){ return rgba_color(147, 112, 219); }
        rgba_color rgba_color::mediumseagreen(){ return rgba_color( 60, 179, 113); }
        rgba_color rgba_color::mediumslateblue(){ return rgba_color(123, 104, 238); }
        rgba_color rgba_color::mediumspringgreen(){ return rgba_color( 0, 250, 154); }
        rgba_color rgba_color::mediumturquoise(){ return rgba_color( 72, 209, 204); }
        rgba_color rgba_color::mediumvioletred(){ return rgba_color(199, 21, 133); }
        rgba_color rgba_color::midnightblue(){ return rgba_color( 25, 25, 112); }
        rgba_color rgba_color::mintcream(){ return rgba_color(245, 255, 250); }
        rgba_color rgba_color::mistyrose(){ return rgba_color(255, 228, 225); }
        rgba_color rgba_color::moccasin(){ return rgba_color(255, 228, 181); }
        rgba_color rgba_color::navajowhite(){ return rgba_color(255, 222, 173); }
        rgba_color rgba_color::navy(){ return rgba_color( 0, 0, 128); }
        rgba_color rgba_color::oldlace(){ return rgba_color(253, 245, 230); }
        rgba_color rgba_color::olive(){ return rgba_color(128, 128, 0); }
        rgba_color rgba_color::olivedrab(){ return rgba_color(107, 142, 35); }
        rgba_color rgba_color::orange(){ return rgba_color(255, 165, 0); }
        rgba_color rgba_color::orangered(){ return rgba_color(255, 69, 0); }
        rgba_color rgba_color::orchid(){ return rgba_color(218, 112, 214); }
        rgba_color rgba_color::palegoldenrod(){ return rgba_color(238, 232, 170); }
        rgba_color rgba_color::palegreen(){ return rgba_color(152, 251, 152); }
        rgba_color rgba_color::paleturquoise(){ return rgba_color(175, 238, 238); }
        rgba_color rgba_color::palevioletred(){ return rgba_color(219, 112, 147); }
        rgba_color rgba_color::papayawhip(){ return rgba_color(255, 239, 213); }
        rgba_color rgba_color::peachpuff(){ return rgba_color(255, 218, 185); }
        rgba_color rgba_color::peru(){ return rgba_color(205, 133, 63); }
        rgba_color rgba_color::pink(){ return rgba_color(255, 192, 203); }
        rgba_color rgba_color::plum(){ return rgba_color(221, 160, 221); }
        rgba_color rgba_color::powderblue(){ return rgba_color(176, 224, 230); }
        rgba_color rgba_color::purple(){ return rgba_color(128, 0, 128); }
        rgba_color rgba_color::red(){ return rgba_color(255, 0, 0); }
        rgba_color rgba_color::rosybrown(){ return rgba_color(188, 143, 143); }
        rgba_color rgba_color::royalblue(){ return rgba_color( 65, 105, 225); }
        rgba_color rgba_color::saddlebrown(){ return rgba_color(139, 69, 19); }
        rgba_color rgba_color::salmon(){ return rgba_color(250, 128, 114); }
        rgba_color rgba_color::sandybrown(){ return rgba_color(244, 164, 96); }
        rgba_color rgba_color::seagreen(){ return rgba_color( 46, 139, 87); }
        rgba_color rgba_color::seashell(){ return rgba_color(255, 245, 238); }
        rgba_color rgba_color::sienna(){ return rgba_color(160, 82, 45); }
        rgba_color rgba_color::silver(){ return rgba_color(192, 192, 192); }
        rgba_color rgba_color::skyblue(){ return rgba_color(135, 206, 235); }
        rgba_color rgba_color::slateblue(){ return rgba_color(106, 90, 205); }
        rgba_color rgba_color::slategray(){ return rgba_color(112, 128, 144); }
        rgba_color rgba_color::slategrey(){ return rgba_color(112, 128, 144); }
        rgba_color rgba_color::snow(){ return rgba_color(255, 250, 250); }
        rgba_color rgba_color::springgreen(){ return rgba_color( 0, 255, 127); }
        rgba_color rgba_color::steelblue(){ return rgba_color( 70, 130, 180); }
        rgba_color rgba_color::tan(){ return rgba_color(210, 180, 140); }
        rgba_color rgba_color::teal(){ return rgba_color( 0, 128, 128); }
        rgba_color rgba_color::thistle(){ return rgba_color(216, 191, 216); }
        rgba_color rgba_color::tomato(){ return rgba_color(255, 99, 71); }
        rgba_color rgba_color::turquoise(){ return rgba_color( 64, 224, 208); }
        rgba_color rgba_color::violet(){ return rgba_color(238, 130, 238); }
        rgba_color rgba_color::wheat(){ return rgba_color(245, 222, 179); }
        rgba_color rgba_color::white(){ return rgba_color(255, 255, 255); }
        rgba_color rgba_color::whitesmoke(){ return rgba_color(245, 245, 245); }
        rgba_color rgba_color::yellow(){ return rgba_color(255, 255, 0); }
        rgba_color rgba_color::yellowgreen(){ return rgba_color(154, 205, 50); }

        rgba_color rgba_color::default_color(const t_string& col_name)
        {
            const std::map< t_string, rgba_color >& colors = get_default_colors();
            std::map< t_string, rgba_color >::const_iterator fit = colors.find(col_name);
            ARENFORCE_MSG( fit != colors.end(), "Unknown color name: %s" )(col_name); 
            return fit->second;
        }

        const std::map< t_string, rgba_color >&  rgba_color::get_default_colors()
        {
            if( m_default_colors.empty()) create_default_colors();
            return m_default_colors;
        }

        void rgba_color::create_default_colors()
        {
            m_default_colors.insert(std::make_pair(t_string(_CT("aliceblue")), rgba_color(240, 248, 255)));
            m_default_colors.insert(std::make_pair(t_string(_CT("antiquewhite")), rgba_color(250, 235, 215)));
            m_default_colors.insert(std::make_pair(t_string(_CT("aqua")), rgba_color(0, 255, 255)));
            m_default_colors.insert(std::make_pair(t_string(_CT("aquamarine")), rgba_color(127, 255, 212)));
            m_default_colors.insert(std::make_pair(t_string(_CT("azure")), rgba_color(240, 255, 255)));
            m_default_colors.insert(std::make_pair(t_string(_CT("beige")), rgba_color(245, 245, 220)));
            m_default_colors.insert(std::make_pair(t_string(_CT("bisque")), rgba_color(255, 228, 196)));
            m_default_colors.insert(std::make_pair(t_string(_CT("black")), rgba_color(0, 0, 0)));
            m_default_colors.insert(std::make_pair(t_string(_CT("blanchedalmond")), rgba_color(255, 235, 205)));
            m_default_colors.insert(std::make_pair(t_string(_CT("blue")), rgba_color(0, 0, 255)));
            m_default_colors.insert(std::make_pair(t_string(_CT("blueviolet")), rgba_color(138, 43, 226)));;
            m_default_colors.insert(std::make_pair(t_string(_CT("brown")), rgba_color(165, 42, 42)));
            m_default_colors.insert(std::make_pair(t_string(_CT("burlywood")), rgba_color(222, 184, 135)));
            m_default_colors.insert(std::make_pair(t_string(_CT("cadetblue")), rgba_color(95, 158, 160)));
            m_default_colors.insert(std::make_pair(t_string(_CT("chartreuse")), rgba_color(127, 255, 0)));
            m_default_colors.insert(std::make_pair(t_string(_CT("chocolate")), rgba_color(210, 105, 30)));
            m_default_colors.insert(std::make_pair(t_string(_CT("coral")), rgba_color(210, 105, 30)));
            m_default_colors.insert(std::make_pair(t_string(_CT("cornflowerblue")), rgba_color(100, 149, 237)));
            m_default_colors.insert(std::make_pair(t_string(_CT("cornsilk")), rgba_color(255, 248, 220)));
            m_default_colors.insert(std::make_pair(t_string(_CT("crimson")), rgba_color(220, 20, 60)));
            m_default_colors.insert(std::make_pair(t_string(_CT("cyan")), rgba_color(0, 255, 255)));
            m_default_colors.insert(std::make_pair(t_string(_CT("darkblue")), rgba_color(0, 0, 139)));
            m_default_colors.insert(std::make_pair(t_string(_CT("darkcyan")), rgba_color(0, 139, 139)));
            m_default_colors.insert(std::make_pair(t_string(_CT("darkgoldenrod")), rgba_color(184, 134, 11)));
            m_default_colors.insert(std::make_pair(t_string(_CT("darkgray")), rgba_color(169, 169, 169)));
            m_default_colors.insert(std::make_pair(t_string(_CT("darkgreen")), rgba_color(0, 100, 0)));
            m_default_colors.insert(std::make_pair(t_string(_CT("darkkhaki")), rgba_color(189, 183, 107)));
            m_default_colors.insert(std::make_pair(t_string(_CT("darkmagenta")), rgba_color(139, 0, 139) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("darkolivegreen")), rgba_color( 85, 107, 47) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("darkorange")), rgba_color(255, 140, 0) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("darkorchid")), rgba_color(153, 50, 204) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("darkred")), rgba_color(139, 0, 0) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("darksalmon")), rgba_color(233, 150, 122) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("darkseagreen")), rgba_color(143, 188, 143) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("darkslateblue")), rgba_color( 72, 61, 139) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("darkslategray")), rgba_color( 47, 79, 79) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("darkslategrey")), rgba_color( 47, 79, 79) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("darkturquoise")), rgba_color( 0, 206, 209) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("darkviolet")), rgba_color(148, 0, 211) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("deeppink")), rgba_color(255, 20, 147) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("deepskyblue")), rgba_color( 0, 191, 255) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("dimgray")), rgba_color(105, 105, 105) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("dimgrey")), rgba_color(105, 105, 105) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("dodgerblue")), rgba_color( 30, 144, 255) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("firebrick")), rgba_color(178, 34, 34) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("floralwhite")), rgba_color(255, 250, 240) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("forestgreen")), rgba_color( 34, 139, 34) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("fuchsia")), rgba_color(255, 0, 255) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("gainsboro")), rgba_color(220, 220, 220) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("ghostwhite")), rgba_color(248, 248, 255) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("gold")), rgba_color(255, 215, 0) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("goldenrod")), rgba_color(218, 165, 32) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("gray")), rgba_color(128, 128, 128) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("green")), rgba_color( 0, 128, 0) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("greenyellow")), rgba_color(173, 255, 47) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("honeydew")), rgba_color(240, 255, 240) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("hotpink")), rgba_color(255, 105, 180) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("indianred")), rgba_color(205, 92, 92) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("indigo")), rgba_color( 75, 0, 130) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("ivory")), rgba_color(255, 255, 240) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("khaki")), rgba_color(240, 230, 140) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("lavender")), rgba_color(230, 230, 250) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("lavenderblush")), rgba_color(255, 240, 245) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("lawngreen")), rgba_color(124, 252, 0) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("lemonchiffon")), rgba_color(255, 250, 205) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("lightblue")), rgba_color(173, 216, 230) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("lightcoral")), rgba_color(240, 128, 128) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("lightcyan")), rgba_color(224, 255, 255) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("lightgoldenrodyellow")), rgba_color(250, 250, 210) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("lightgray")), rgba_color(211, 211, 211) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("lightgreen")), rgba_color(144, 238, 144) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("lightgrey")), rgba_color(211, 211, 211) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("lightpink")), rgba_color(255, 182, 193) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("lightsalmon")), rgba_color(255, 160, 122) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("lightseagreen")), rgba_color( 32, 178, 170) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("lightskyblue")), rgba_color(135, 206, 250) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("lightslategray")), rgba_color(119, 136, 153) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("lightslategrey")), rgba_color(119, 136, 153) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("lightsteelblue")), rgba_color(176, 196, 222) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("lightyellow")), rgba_color(255, 255, 224) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("lime")), rgba_color( 0, 255, 0) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("limegreen")), rgba_color( 50, 205, 50) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("linen")), rgba_color(250, 240, 230) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("magenta")), rgba_color(255, 0, 255) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("maroon")), rgba_color(128, 0, 0) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("mediumaquamarine")), rgba_color(102, 205, 170) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("mediumblue")), rgba_color( 0, 0, 205) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("mediumorchid")), rgba_color(186, 85, 211) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("mediumpurple")), rgba_color(147, 112, 219) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("mediumseagreen")), rgba_color( 60, 179, 113) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("mediumslateblue")), rgba_color(123, 104, 238) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("mediumspringgreen")), rgba_color( 0, 250, 154) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("mediumturquoise")), rgba_color( 72, 209, 204) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("mediumvioletred")), rgba_color(199, 21, 133) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("midnightblue")), rgba_color( 25, 25, 112) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("mintcream")), rgba_color(245, 255, 250) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("mistyrose")), rgba_color(255, 228, 225) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("moccasin")), rgba_color(255, 228, 181) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("navajowhite")), rgba_color(255, 222, 173) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("navy")), rgba_color( 0, 0, 128) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("oldlace")), rgba_color(253, 245, 230) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("olive")), rgba_color(128, 128, 0) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("olivedrab")), rgba_color(107, 142, 35) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("orange")), rgba_color(255, 165, 0) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("orangered")), rgba_color(255, 69, 0) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("orchid")), rgba_color(218, 112, 214) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("palegoldenrod")), rgba_color(238, 232, 170) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("palegreen")), rgba_color(152, 251, 152) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("paleturquoise")), rgba_color(175, 238, 238) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("palevioletred")), rgba_color(219, 112, 147) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("papayawhip")), rgba_color(255, 239, 213) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("peachpuff")), rgba_color(255, 218, 185) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("peru")), rgba_color(205, 133, 63) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("pink")), rgba_color(255, 192, 203) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("plum")), rgba_color(221, 160, 221) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("powderblue")), rgba_color(176, 224, 230) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("purple")), rgba_color(128, 0, 128) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("red")), rgba_color(255, 0, 0) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("rosybrown")), rgba_color(188, 143, 143) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("royalblue")), rgba_color( 65, 105, 225) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("saddlebrown")), rgba_color(139, 69, 19) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("salmon")), rgba_color(250, 128, 114) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("sandybrown")), rgba_color(244, 164, 96) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("seagreen")), rgba_color( 46, 139, 87) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("seashell")), rgba_color(255, 245, 238) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("sienna")), rgba_color(160, 82, 45) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("silver")), rgba_color(192, 192, 192) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("skyblue")), rgba_color(135, 206, 235) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("slateblue")), rgba_color(106, 90, 205) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("slategray")), rgba_color(112, 128, 144) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("slategrey")), rgba_color(112, 128, 144) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("snow")), rgba_color(255, 250, 250) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("springgreen")), rgba_color( 0, 255, 127) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("steelblue")), rgba_color( 70, 130, 180) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("tan")), rgba_color(210, 180, 140) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("teal")), rgba_color( 0, 128, 128) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("thistle")), rgba_color(216, 191, 216) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("tomato")), rgba_color(255, 99, 71) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("turquoise")), rgba_color( 64, 224, 208) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("violet")), rgba_color(238, 130, 238) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("wheat")), rgba_color(245, 222, 179) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("white")), rgba_color(255, 255, 255) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("whitesmoke")), rgba_color(245, 245, 245) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("yellow")), rgba_color(255, 255, 0) ));
            m_default_colors.insert(std::make_pair(t_string(_CT("yellowgreen")), rgba_color(154, 205, 50) ));

        }
    }
}
