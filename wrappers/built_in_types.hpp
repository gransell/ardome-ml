
namespace boost
{
    
    /** 
        <bindgen>
    		<attribute name="visibility" value="private"></attribute>
    	</bindgen>
    */
    template< class T >
    class shared_ptr {};
	
    /** 
        <bindgen>
    		<attribute name="visibility" value="private"></attribute>
    	</bindgen>
    */
    template< class T >
    class weak_ptr {};

    /** 
        <bindgen>
    		<attribute name="visibility" value="private"></attribute>
    	</bindgen>
    */
    template< class T >
	class equality_comparable {};
	
    /** 
        <bindgen>
    		<attribute name="visibility" value="private"></attribute>
    	</bindgen>
    */
    template< class T >
	class addable {};
	
    /** 
        <bindgen>
    		<attribute name="visibility" value="private"></attribute>
    	</bindgen>
    */
    template< class T >
	class subtractable {};
	
	/** 
        <bindgen>
    		<attribute name="visibility" value="private"></attribute>
    	</bindgen>
    */
    template< class T >
    class rational {};
	
    /** 
        <bindgen>
    		<attribute name="visibility" value="private"></attribute>
    	</bindgen>
    */
	class recursive_mutex {};
	
	typedef int int32_t;
    typedef unsigned int uint32_t;
    typedef long int64_t;
	typedef unsigned char uint8_t;
    typedef unsigned short uint16_t;
	
    /** 
        <bindgen>
    		<attribute name="visibility" value="private"></attribute>
    	</bindgen>
    */
    class format {};
	
	/** 	<bindgen><attribute name="visibility" value="private"></attribute></bindgen> */
    class format {};
	
	/** 	<bindgen><attribute name="visibility" value="private"></attribute></bindgen> */
    class wformat {};
	
	namespace filesystem {
		
	    /** 
	        <bindgen>
	    		<attribute name="visibility" value="private"></attribute>
	    	</bindgen>
	    */
	    class ofstream {};
		
		/** 
	        <bindgen>
	    		<attribute name="visibility" value="private"></attribute>
	    	</bindgen>
	    */
	    class wofstream {};
		
		
	    /** 
	        <bindgen>
	    		<attribute name="visibility" value="private"></attribute>
	    	</bindgen>
	    */
	    class fstream {};
		
		/** 
	        <bindgen>
	    		<attribute name="visibility" value="private"></attribute>
	    	</bindgen>
	    */
	    class wfstream {};
		
		
	    /** 
	        <bindgen>
	    		<attribute name="visibility" value="private"></attribute>
	    	</bindgen>
	    */
	    class ifstream {};

/** 
	        <bindgen>
	    		<attribute name="visibility" value="private"></attribute>
	    	</bindgen>
			*/
	    class wifstream {};
		
	    /** 
	        <bindgen>
	    		<attribute name="visibility" value="private"></attribute>
	    	</bindgen>
	    */
	    class path {};
		
		/** 
	        <bindgen>
	    		<attribute name="visibility" value="private"></attribute>
	    	</bindgen>
	    */
	    class wpath {};
		
	    /** 
	        <bindgen>
	    		<attribute name="visibility" value="private"></attribute>
	    	</bindgen>
	    */
	    class directory_iterator {};
		
		/** 
	        <bindgen>
	    		<attribute name="visibility" value="private"></attribute>
	    	</bindgen>
	    */
	    class wdirectory_iterator {};
	}
}

namespace std
{
    
    /** 
        <bindgen>
    		<attribute name="visibility" value="private"></attribute>
    		<attribute name="convert" value="yes"></attribute>
    	</bindgen>
    */
	class string {};

    /** 
        <bindgen>
    		<attribute name="visibility" value="private"></attribute>
    		<attribute name="convert" value="yes"></attribute>
    	</bindgen>
    */
    class wstring {};
    
    /** 
        <bindgen>
    		<attribute name="visibility" value="private"></attribute>
    		<attribute name="convert" value="yes"></attribute>
    	</bindgen>
    */
    template< class T >
    class basic_string {};

    /** 
        <bindgen>
    		<attribute name="visibility" value="private"></attribute>
    		<attribute name="convert" value="yes"></attribute>
    	</bindgen>
    */
    template< class T >
    class deque {};
    
    
    /** 
        <bindgen>
    		<attribute name="visibility" value="private"></attribute>
    		<attribute name="convert" value="yes"></attribute>
    	</bindgen>
    */
    template< class T >
    class vector {};
    
    /** 
        <bindgen>
    		<attribute name="visibility" value="private"></attribute>
    		<attribute name="convert" value="yes"></attribute>
    	</bindgen>
    */
    template< class K, class V >
    class map {};
    
    
    /** 
        <bindgen>
    		<attribute name="visibility" value="private"></attribute>
    	</bindgen>
    */
    class iostream {};
    
	/** 
        <bindgen>
    		<attribute name="visibility" value="private"></attribute>
    	</bindgen>
		*/
    class wiostream {};
    	
    /** 
        <bindgen>
    		<attribute name="visibility" value="private"></attribute>
    	</bindgen>
    */
    class stringstream {};

	/** 
        <bindgen>
    		<attribute name="visibility" value="private"></attribute>
    	</bindgen>
    */
    class wstringstream {};

    /** 
        <bindgen>
    		<attribute name="visibility" value="private"></attribute>
    	</bindgen>
    */
    class ostringstream {};

	/** 
        <bindgen>
    		<attribute name="visibility" value="private"></attribute>
    	</bindgen>
    */
    class wostringstream {};

    /** 
        <bindgen>
    		<attribute name="visibility" value="private"></attribute>
    	</bindgen>
    */
    class istringstream {};

	/** 
        <bindgen>
    		<attribute name="visibility" value="private"></attribute>
    	</bindgen>
    */
    class wistringstream {};

	/** 
	    <bindgen>
			<attribute name="visibility" value="private"></attribute>
		</bindgen>
*/
	class ofstream {};
	
	/** 
	    <bindgen>
			<attribute name="visibility" value="private"></attribute>
		</bindgen>
*/
	class wofstream {};
	
    /** 
        <bindgen>
    		<attribute name="visibility" value="private"></attribute>
    	</bindgen>
    */
    class ifstream {};
	
	/** 
        <bindgen>
    		<attribute name="visibility" value="private"></attribute>
    	</bindgen>
    */
    class wifstream {};
	
    /** 
        <bindgen>
    		<attribute name="visibility" value="private"></attribute>
    	</bindgen>
    */
    class fstream {};
	
	/** 
        <bindgen>
    		<attribute name="visibility" value="private"></attribute>
    	</bindgen>
    */
    class wfstream {};
	
    /** 
        <bindgen>
    		<attribute name="visibility" value="private"></attribute>
    	</bindgen>
    */
    class ostream {};
	
	/** 
        <bindgen>
    		<attribute name="visibility" value="private"></attribute>
    	</bindgen>
    */
    class wostream {};
	
    /** 
        <bindgen>
    		<attribute name="visibility" value="private"></attribute>
    	</bindgen>
    */
    class istream {};
	
	/** 
        <bindgen>
    		<attribute name="visibility" value="private"></attribute>
    	</bindgen>
    */
    class wistream {};    
}