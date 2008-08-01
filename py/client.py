import socket

class client:
	"""Simple blocking client for the aml server."""

	def __init__( self, server = 'localhost', port = 55378 ):
		"""Construct and connect to the server (will throw an exception if
		server is unavailable."""

		self.server = server
		self.port = port
		self.socket = socket.socket( socket.AF_INET, socket.SOCK_STREAM )
		self.socket.connect( ( server, port ) )

	def send( self, input ):
		"""Send a line to the server - it should have no line separators. (or 
		if it should, it better make sure it damn well has them...)"""

		sent = 0
		input += '\r\n'
		while sent != len( input ):
			bytes = self.socket.send( input[ sent: ] )
			if bytes == 0:
				raise Exception, "Send failed"
			sent += bytes

	def receive( self ):
		"""Receive the response to the previously sent line."""

		list = [ ]
		result = ''
		finished = False
		while not finished:
			chunk = self.socket.recv( 1024 )
			if chunk == '':
				raise Exception, "Recv failed"
			result += chunk
			list += result.split( '\r\n' )
			result = list.pop( -1 )
			if result == '':
				if list[ -1 ] == 'OK':
					list.pop( -1 )
					finished = True
				else:
					finished = list[ -1 ].startswith( 'ERROR: ' )
		return list

	def shell( self ):
		"""Really shouldn't be here (subclass of client_shell needed perhaps?)."""

		import readline
		readline.parse_and_bind("tab: complete")
		try:
			while True:
				try:
					input = raw_input( 'aml@%s:%d> ' % ( self.server, self.port ) ).strip( )
					# Gack! drag and drop from nautilus...
					input = input.replace( "\'", "\"" ).replace( "\"\\\"\"", "\'" )
					if input != '':
						self.send( input )
						list = self.receive( )
						for entry in list:
							print entry
				except KeyboardInterrupt:
					try:
						self.send( "reset!" )
					except Exception, e:
						print '\n' + str( e )
		except EOFError:
			print

