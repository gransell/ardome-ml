import aml

import socket
import threading
import select
import os

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
		self.stack = aml.thread_stack( parent.player, self.output )
		self.stack.include( 'shell.aml' )
		self.msg = ''
		self.pending = [ ]

	def read( self ):
		"""Read a chunk of data, parse complete lines and collect results."""

		chunk = self.socket.recv( 1024 )
		if chunk == '':
			self.parent.unregister_client( self.socket )
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
		while total < len( message ):
			sent = self.socket.send( message[ total: ] )
			if sent == 0:
				return total
			total += sent
		self.socket.send( '\r\n' )
		return total

	def write( self ):
		"""Send all pending data back to the client."""

		while len( self.pending ):
			for result in self.pending:
				if self.send( result ) != len( result ):
					self.parent.unregister_client( self.socket )
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

	def read( self ):
		"""Client connection available."""

		client, address = self.socket.accept( )
		self.parent.register_client( client, address )

	def write( self ):
		"""We should never write to this socket..."""

		pass

class server( threading.Thread ):

	"""AML Server implementation."""

	def __init__( self, player, port = 55378 ):
		"""Constructor for the server and associated thread."""

		threading.Thread.__init__( self )
		self.player = player
		self.port = port
		self.sockets = { }
		self.writer = { }
		self.setDaemon( True )

	def register_client( self, client, address ):
		"""Register a new client."""

		self.sockets[ client ] = client_handler( self, client )

	def unregister_client( self, client ):
		"""Remove the client."""

		if client in self.sockets.keys( ):
			self.sockets.pop( client )
		if client in self.writer.keys( ):
			self.writer.pop( client )

	def register_writer( self, client ):
		"""There is pending data associated to the client."""

		self.writer[ client ] = self.sockets[ client ]

	def unregister_writer( self, client ):
		"""There is no pending data associated to the client."""

		if client in self.writer.keys( ):
			self.writer.pop( client )

	def run( self ):
		"""The thread method."""

		server = socket.socket( socket.AF_INET, socket.SOCK_STREAM )
		self.sockets[ server ] = server_handler( self, server )

		try:
			while True:
				readers, writers, errors = select.select( self.sockets.keys(), self.writer.keys(), [] , 60 )

				for error in errors:
					if error in self.sockets.keys( ):
						self.sockets.pop( error )
					if error in self.writer.keys( ):
						self.writer.pop( error )
	
				for writer in writers:
					if writer in self.writer.keys( ):
						self.writer[ writer ].write( )
	
				for reader in readers:
					if reader in self.sockets.keys( ):
						self.sockets[ reader ].read( )

		except Exception, e:
			print e
			server.close( )
