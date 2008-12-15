
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef CONTENT_HANDLER_MSXML_INC_
#define CONTENT_HANDLER_MSXML_INC_

#if _MSC_VER
#pragma warning ( push )
#	pragma warning ( disable:4192 )
#endif

#import <msxml4.dll> raw_interfaces_only

#if _MSC_VER
#	pragma warning ( default:4192 )
#pragma warning ( pop )
#endif

#include <openpluginlib/pl/opl_parser_action.hpp>

namespace olib { namespace openpluginlib {

//
class content_handler_msxml : public MSXML2::ISAXContentHandler
{
public:
	explicit content_handler_msxml( );
	virtual ~content_handler_msxml( );

	long			__stdcall QueryInterface( const struct _GUID&, void** );
	unsigned long	__stdcall AddRef( void );
	unsigned long	__stdcall Release( void );

	virtual HRESULT STDMETHODCALLTYPE putDocumentLocator( MSXML2::ISAXLocator __RPC_FAR* locator );

	virtual HRESULT STDMETHODCALLTYPE startDocument( void );
	virtual HRESULT STDMETHODCALLTYPE endDocument( void );

	virtual HRESULT STDMETHODCALLTYPE startPrefixMapping( unsigned short __RPC_FAR* pwchPrefix,
														  int cchPrefix,
														  unsigned short __RPC_FAR* pwchUri,
														  int cchUri );

	virtual HRESULT STDMETHODCALLTYPE endPrefixMapping( unsigned short __RPC_FAR* pwchPrefix,
														int cchPrefix );

	virtual HRESULT STDMETHODCALLTYPE startElement( unsigned short __RPC_FAR* pwchNamespaceUri,
													int cchNamespaceUri,
													unsigned short __RPC_FAR* pwchLocalName,
													int cchLocalName,
													unsigned short __RPC_FAR* pwchRawName,
													int cchRawName,
													MSXML2::ISAXAttributes __RPC_FAR* pAttributes );

	virtual HRESULT STDMETHODCALLTYPE endElement( unsigned short __RPC_FAR* pwchNamespaceUri,
												  int cchNamespaceUri,
												  unsigned short __RPC_FAR* pwchLocalName,
												  int cchLocalName,
												  unsigned short __RPC_FAR* pwchRawName,
												  int cchRawName );

	virtual HRESULT STDMETHODCALLTYPE characters( unsigned short __RPC_FAR* pwchChars, int cchChars );
	virtual HRESULT STDMETHODCALLTYPE ignorableWhitespace( unsigned short __RPC_FAR* pwchChars, int cchChars );
	virtual HRESULT STDMETHODCALLTYPE processingInstruction( unsigned short __RPC_FAR* pwchTarget,
															 int cchTarget,
															 unsigned short __RPC_FAR* pwchData,
															 int cchData );

	virtual HRESULT STDMETHODCALLTYPE skippedEntity( unsigned short __RPC_FAR* pwchName, int cchName );

	void								set_branch_path( const boost::filesystem::path& branch_path )
	{ action_.set_branch_path( branch_path ); }
	const actions::opl_parser_action&	get_action( )
	{ return action_; }
	
private:
	actions::opl_parser_action action_;
	ULONG refcnt_;
};

} }

#endif
