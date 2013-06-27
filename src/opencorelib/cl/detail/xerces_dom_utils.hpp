#include <xercesc/dom/DOMErrorHandler.hpp>
#include <xercesc/dom/DOMUserDataHandler.hpp>
#include <xercesc/internal/XMLScanner.hpp>

namespace olib { namespace opencorelib { namespace xml { namespace dom {

	xerces_string source_location_key();

	//Custom subclass to the normal DOMParser, which records line and
	//column number for each node.
	//General concept taken from: http://markmail.org/message/fofympr5oendfnxx
	class cl_dom_parser : public XercesDOMParser
	{
	public:
		cl_dom_parser();
		virtual ~cl_dom_parser();

		//Base class override
		virtual void startElement(
			const XMLElementDecl &elem_decl,
			const unsigned int url_id,
			const XMLCh* const elem_prefix,
			const RefVectorOf<XMLAttr> &attr_list,
			const unsigned int attr_count,
			const bool is_empty,
			const bool is_root );

		class source_location
		{
		public:
			source_location( int ln );

			void add_ref();
			void release();

			int line_;
			size_t ref_count_;
		};

	protected:
		class error_handler : public ErrorHandler
		{
			virtual void warning( const SAXParseException &exc );
			virtual void error( const SAXParseException &exc );
			virtual void fatalError( const SAXParseException &exc );
			virtual void resetErrors();
		};

	protected:
		error_handler error_handler_;
	};
} } } }

