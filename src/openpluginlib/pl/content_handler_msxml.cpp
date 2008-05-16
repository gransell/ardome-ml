
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <openpluginlib/pl/content_handler_msxml.hpp>

namespace olib { namespace openpluginlib {

//
content_handler_msxml::content_handler_msxml( )
	: refcnt_( 0 )
{ }

//
content_handler_msxml::~content_handler_msxml( )
{ }

//
long __stdcall content_handler_msxml::QueryInterface( const struct _GUID& riid, void** ppvObject )
{
	*ppvObject = NULL;
	if( riid == IID_IUnknown || riid == __uuidof( MSXML2::ISAXContentHandler ) )
		*ppvObject = this;

	if( *ppvObject )
	{
		AddRef( );

		return S_OK;
	}

	return E_NOINTERFACE;
}
		
unsigned long __stdcall content_handler_msxml::AddRef( void )
{ return ++refcnt_; }

unsigned long __stdcall content_handler_msxml::Release( void )
{
	--refcnt_;
	if( refcnt_ == 0 )
	{
		delete this;

		return 0;
	}

	return refcnt_;
}

HRESULT STDMETHODCALLTYPE content_handler_msxml::putDocumentLocator( MSXML2::ISAXLocator __RPC_FAR* /*locator*/ )
{ return S_OK; }

HRESULT STDMETHODCALLTYPE content_handler_msxml::startDocument( void )
{ return S_OK; }	

HRESULT STDMETHODCALLTYPE content_handler_msxml::endDocument( void )
{ return S_OK; }
			
HRESULT STDMETHODCALLTYPE content_handler_msxml::startPrefixMapping( unsigned short __RPC_FAR* /*pwchPrefix*/,
																   		 int /*cchPrefix*/,
																   		 unsigned short __RPC_FAR* /*pwchUri*/,
																   		 int /*cchUri*/ )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE content_handler_msxml::endPrefixMapping( unsigned short __RPC_FAR* /*pwchPrefix*/,
																 	   int /*cchPrefix*/ )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE content_handler_msxml::startElement( unsigned short __RPC_FAR* /*pwchNamespaceUri*/,
															 	   int /*cchNamespaceUri*/,
															 	   unsigned short __RPC_FAR* pwchLocalName,
															 	   int cchLocalName,
															 	   unsigned short __RPC_FAR* /*pwchRawName*/,
															 	   int /*cchRawName*/,
															 	   MSXML2::ISAXAttributes __RPC_FAR* pAttributes )
{
	action_.set_attrs( pAttributes );
	action_.dispatch( wstring( reinterpret_cast<wchar_t*>( pwchLocalName ), cchLocalName ) );

	return S_OK;
}

HRESULT STDMETHODCALLTYPE content_handler_msxml::endElement( unsigned short __RPC_FAR* /*pwchNamespaceUri*/,
														   		 int /*cchNamespaceUri*/,
														   		 unsigned short __RPC_FAR* /*pwchLocalName*/,
														   		 int /*cchLocalName*/,
														   		 unsigned short __RPC_FAR* /*pwchRawName*/,
														   		 int /*cchRawName*/ )
{
	return S_OK;
}
			
HRESULT STDMETHODCALLTYPE content_handler_msxml::characters( unsigned short __RPC_FAR* /*pwchChars*/, int /*cchChars*/ )
{
	return S_OK;
}
			
HRESULT STDMETHODCALLTYPE content_handler_msxml::ignorableWhitespace( unsigned short __RPC_FAR* /*pwchChars*/, int /*cchChars*/ )
{
	return S_OK;
}
			
HRESULT STDMETHODCALLTYPE content_handler_msxml::processingInstruction( unsigned short __RPC_FAR* /*pwchTarget*/,
																	  		int /*cchTarget*/,
																	  		unsigned short __RPC_FAR* /*pwchData*/,
																	  		int /*cchData*/ )
{
	return S_OK;
}
																		 
HRESULT STDMETHODCALLTYPE content_handler_msxml::skippedEntity( unsigned short __RPC_FAR* /*pwchName*/, int /*cchName*/ )
{
	return S_OK;
}

} }
