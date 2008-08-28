import aml

import socket
import threading
import select
import os
import urllib
import sys

class client_handler:

	"""Handler for the client socket. Each client has their own stack and 
	certain words are triggered to interact with the shared player instance 
	which is provided in the ctor."""

	def __init__( self, parent, client ):
		"""Constructor."""

		self.parent = parent
		self.socket = client
		self.socket.settimeout( 0 )
		self.socket.setblocking( 0 )
		self.msg = ''
		self.pending = [ ]
		self.stack = aml.thread_stack( parent.player, self.output )
		self.stack.include( 'shell.aml' )
		self.parent.register_socket( client, self )

	def read( self ):
		"""Read a chunk of data, parse complete lines and collect results."""

		chunk = self.socket.recv( 1024 )
		if chunk == '':
			self.parent.unregister_socket( self.socket )
		else:
			self.msg += chunk
			if self.msg.find( '\r\n' ) != -1:
				lines = self.msg.split( '\r\n' )
				self.msg = lines.pop( -1 )

				for line in lines:
					try:
						if line != '':
							self.stack.define( line )
							self.pending += [ 'OK' ]
					except Exception, e:
						self.pending += [ 'ERROR: ' + str( e ) ]

				if len( self.pending ):
					self.parent.register_writer( self.socket )

	def send( self, message ):
		"""Send a single message back to the client."""

		total = 0
		message = message.replace( '\r\n', '\n' ).replace( '\n', '\r\n' )
		if not message.endswith( '\n' ):
			message += '\r\n'
		while total < len( message ):
			sent = self.socket.send( message[ total: ] )
			if sent == 0:
				return False
			total += sent
		return True

	def write( self ):
		"""Send all pending data back to the client."""

		while len( self.pending ):
			for result in self.pending:
				if self.send( result ) == False:
					self.parent.unregister_socket( self.socket )
					return
			self.pending = [ ]
		
		if len( self.pending ) == 0:
			self.parent.unregister_writer( self.socket )

	def output( self, string ):
		"""Callback to collect output."""

		self.pending += [ string ]

class server_handler:

	"""Handler for the server socket."""

	def __init__( self, parent, server ):
		"""Construct the server."""

		self.parent = parent
		self.socket = server
		self.socket.bind( ( '', parent.port ) )
		self.socket.listen( 5 )
		self.socket.setblocking( 0 )
		self.socket.settimeout( 0 )

	def __del__( self ):
		self.socket = None

	def read( self ):
		"""Client connection available."""

		client, address = self.socket.accept( )
		client_handler( self.parent, client )

	def write( self ):
		"""We should never write to this socket..."""

		pass

class http_client_handler:

	"""Handler for the http client socket."""

	def __init__( self, parent, client ):
		"""Constructor."""

		self.parent = parent
		self.socket = client
		self.socket.settimeout( 0 )
		self.socket.setblocking( 0 )
		self.msg = ''
		self.pending = ''
		self.parent.register_socket( client, self )

	def parse( self, chunk ):
		lines = chunk.split( '\r\n' )
		if lines[ 0 ].startswith( 'GET ' ):
			tokens = lines[ 0 ].split( )
			request = urllib.unquote( tokens[ 1 ] )
			if request.startswith( '/?src=' ):
				media = request[ 6: ].split( '&' )
				stack = self.parent.stack_acquire( )
				stack.push( 'aml:' + media[ 0 ] )
				for arg in media[ 1: ]:
					if arg.startswith( 'frames=' ):
						stack.push( arg )
				stack.push( 'add' )
				self.parent.stack_release( stack )

	def read( self ):
		"""Read a chunk of data, parse complete lines and collect results."""

		chunk = self.socket.recv( 1024 )
		if chunk == '':
			self.parent.unregister_socket( self.socket )
		else:
			self.msg += chunk
			if self.msg.endswith( '\r\n\r\n' ):
				self.parse( self.msg )
				self.pending = 'HTTP/1.0 204 Empty\r\n\r\n'
				self.parent.register_writer( self.socket )

	def write( self ):
		"""Send all pending data back to the client."""
		total = 0
		message = self.pending
		while total < len( message ):
			sent = self.socket.send( message[ total: ] )
			if sent == 0:
				return False
			total += sent
		self.pending = ''
		self.parent.unregister_socket( self.socket )
		return True

	def output( self, msg ):
		print msg,

class http_server_handler:

	"""Handler for the http server socket."""

	def __init__( self, parent, server ):
		"""Construct the server."""

		self.parent = parent
		self.socket = server
		self.socket.bind( ( '', parent.port + 1 ) )
		self.socket.listen( 5 )
		self.socket.setblocking( 0 )
		self.socket.settimeout( 0 )

	def __del__( self ):
		self.socket = None

	def read( self ):
		"""Client connection available."""

		client, address = self.socket.accept( )
		http_client_handler( self.parent, client )

	def write( self ):
		"""We should never write to this socket..."""

		pass

class server( threading.Thread ):

	"""AML Server implementation."""

	def __init__( self, player, port = 55378, http_port = -1 ):
		"""Constructor for the server and associated thread."""

		threading.Thread.__init__( self )
		self.player = player
		self.port = port
		self.http_port = http_port
		self.sockets = { }
		self.writer = { }
		self.setDaemon( True )
		self.pool = [ ]

		self.server = socket.socket( socket.AF_INET, socket.SOCK_STREAM )
		self.server.setsockopt( socket.SOL_SOCKET, socket.SO_REUSEADDR, 1 )
		self.register_socket( self.server, server_handler( self, self.server ) )

		if http_port != -1:
			self.http_server = socket.socket( socket.AF_INET, socket.SOCK_STREAM )
			self.http_server.setsockopt( socket.SOL_SOCKET, socket.SO_REUSEADDR, 1 )
			self.register_socket( self.http_server, http_server_handler( self, self.http_server ) )

	def stack_acquire( self ):
		if len( self.pool ) == 0:
			self.pool = [ aml.thread_stack( self.player ) ]
			self.pool[ 0 ].include( 'shell.aml' )
		return self.pool.pop( )

	def stack_release( self, stack ):
		self.pool.append( stack )

	def register_socket( self, socket, handler ):
		"""Register a new socket."""

		self.sockets[ socket ] = handler

	def unregister_socket( self, sock ):
		"""Remove the socket."""

		if sock in self.sockets.keys( ):
			self.sockets.pop( sock )
		if sock in self.writer.keys( ):
			self.writer.pop( sock )
		sock.close( )

	def register_writer( self, client ):
		"""There is pending data associated to the client."""

		self.writer[ client ] = self.sockets[ client ]

	def unregister_writer( self, client ):
		"""There is no pending data associated to the client."""

		if client in self.writer.keys( ):
			self.writer.pop( client )

	def run( self ):
		"""The thread method."""

		while True:
			try:
				readers, writers, errors = select.select( self.sockets.keys( ), self.writer.keys( ), [ ], 60 )

				for error in errors:
					self.unregister_socket( error )

				for writer in writers:
					if writer in self.writer.keys( ):
						self.writer[ writer ].write( )

				for reader in readers:
					if reader in self.sockets.keys( ):
						self.sockets[ reader ].read( )

			except Exception, e:
				print e

				for sock in self.sockets.keys( ):
					readers, writers, errors = select.select( [ sock ], [ ], [ ], 0 )
					for error in errors:
						error.close( )

