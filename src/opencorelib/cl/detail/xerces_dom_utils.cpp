#include "precompiled_headers.hpp"
#include "../xerces_utilities.hpp"

#if defined (XERCES_CPP_NAMESPACE)
	using namespace XERCES_CPP_NAMESPACE;
#endif

#include "xerces_dom_utils.hpp"

namespace olib { namespace opencorelib { namespace xml { namespace dom {

	xerces_string source_location_key()
	{
		return L2("opencorelib_xml_dom_source_location");
	}

	namespace
	{
		//Handles the lifetime of user data attached to nodes
		class user_data_handler : public DOMUserDataHandler
		{
			//Interface implementation
			virtual void handle(
				DOMOperationType operation,
				const XMLCh *const key,
				void *data,
				const DOMNode *src,
				const DOMNode *dst )
			{
				cl_dom_parser::source_location *sloc =
					static_cast< cl_dom_parser::source_location * >( data );

				if( sloc == NULL )
					return;

				//Increase/decrease refcount of the source_location
				//data when the node is cloned/deleted.
				switch (operation)
				{
					case NODE_CLONED:
					case NODE_IMPORTED:
						sloc->add_ref();
						break;
					case NODE_DELETED:
						sloc->release();
						break;
					case NODE_RENAMED:
						break;
				}
			}
		};

		//The user data handler has no internal state; it could
		//essentially have been a free function instead, so
		//no real harm in making it static.
		static user_data_handler s_the_user_data_handler;
	}


	//Error handler for the DOM parser
	void cl_dom_parser::error_handler::warning( const SAXParseException &exc )
	{
		ARLOG_WARN( "XML DOM parsing warning at line %1%: %2%" )
			( exc.getLineNumber() )
			( exc.getMessage() );
	}

	void cl_dom_parser::error_handler::error( const SAXParseException &exc )
	{
		throw exc;
	}

	void cl_dom_parser::error_handler::fatalError( const SAXParseException &exc )
	{
		throw exc;
	}

	void cl_dom_parser::error_handler::resetErrors()
	{
		//Do nothing
	}

	cl_dom_parser::source_location::source_location( int ln )
		: line_( ln )
		, ref_count_( 1 )
	{
	}

	void cl_dom_parser::source_location::add_ref()
	{
		++ref_count_;
	}

	void cl_dom_parser::source_location::release()
	{
		--ref_count_;
		if( ref_count_ == 0 )
		{
			delete this;
		}
	}

	cl_dom_parser::cl_dom_parser()
	{
		setErrorHandler( &error_handler_ );
	}

	cl_dom_parser::~cl_dom_parser()
	{
	}

	void cl_dom_parser::startElement(
		const XMLElementDecl &elem_decl,
		const unsigned int url_id,
		const XMLCh* const elem_prefix,
		const RefVectorOf<XMLAttr> &attr_list,
		const unsigned int attr_count,
		const bool is_empty,
		const bool is_root )
	{
		XercesDOMParser::startElement(
			elem_decl, url_id, elem_prefix,
			attr_list, attr_count,
			is_empty, is_root );

		//Create a custom user data object containing the line and
		//column number information.
		const Locator *locator = getScanner()->getLocator();
		source_location *loc = new source_location( locator->getLineNumber() );

		XercesDOMParser::fCurrentNode->setUserData(
			source_location_key().c_str(), loc, &s_the_user_data_handler );
	}

} } } }

