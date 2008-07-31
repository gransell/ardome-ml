import aml

import socket
import threading
import select
import os

class client_handler:

	def __init__( self, parent, client ):
		self.parent = parent
		self.socket = client
		self.socket.settimeout( 0 )
		self.socket.setblocking( 0 )
		self.stack = aml.thread_stack( parent.player )
		self.stack.include( 'shell.aml' )
		self.msg = ''
		self.pending = [ ]

	def read( self ):
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
						self.stack.define( line )
						self.pending += [ 'OK' ]
					except Exception, e:
						self.pending += [ str( e ) ]
				if len( self.pending ):
					self.parent.register_writer( self.socket )

	def send( self, message ):
		total = 0
		while total < len( message ):
			sent = self.socket.send( message[ total: ] )
			if sent == 0:
				return total
			total += sent
		self.socket.send( '\r\n' )
		return total

	def write( self ):
		while len( self.pending ):
			for result in self.pending:
				if self.send( result ) != len( result ):
					self.parent.unregister_client( self.socket )
					return
			self.pending = [ ]
		
		if len( self.pending ) == 0:
			self.parent.unregister_writer( self.socket )


class server_handler:

	def __init__( self, parent, server ):
		self.parent = parent
		self.socket = server
		self.socket.bind( ( '', parent.port ) )
		self.socket.listen( 5 )
		self.socket.setblocking( 0 )
		self.socket.settimeout( 0 )

	def read( self ):
		client, address = self.socket.accept( )
		self.parent.register_client( client, address )

	def write( self ):
		pass

class server( threading.Thread ):

	def __init__( self, player, port = 55378 ):
		threading.Thread.__init__( self )
		self.player = player
		self.port = port
		self.handlers = { }
		self.waiting = { }
		self.setDaemon( True )

	def register_client( self, client, address ):
		self.handlers[ client ] = client_handler( self, client )

	def unregister_client( self, client ):
		if client in self.handlers.keys( ):
			self.handlers.pop( client )
		if client in self.waiting.keys( ):
			self.waiting.pop( client )

	def register_writer( self, client ):
		self.waiting[ client ] = self.handlers[ client ]

	def unregister_writer( self, client ):
		if client in self.waiting.keys( ):
			self.waiting.pop( client )

	def run( self ):
		"""The thread method."""

		server = socket.socket( socket.AF_INET, socket.SOCK_STREAM )
		self.handlers[ server ] = server_handler( self, server )

		try:
			while True:
				readers, writers, errors = select.select( self.handlers.keys( ), self.waiting.keys( ), [ ] , 60 )

				for error in errors:
					if error in self.handlers.keys( ):
						self.handlers.pop( error )
					if error in self.waiting.keys( ):
						self.waiting.pop( error )
	
				for writer in writers:
					if writer in self.waiting.keys( ):
						self.waiting[ writer ].write( )
	
				for reader in readers:
					if reader in self.handlers.keys( ):
						self.handlers[ reader ].read( )

		except Exception, e:
			print e
			server.close( )

