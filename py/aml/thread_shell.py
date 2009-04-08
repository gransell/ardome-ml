import aml
import threading
import readline

class thread_shell( threading.Thread ):
	"""Provides an interactive shell which runs as a background thread to a
	thread safe player instance."""

	def __init__( self, player ):
		"""Constructor - initialises the thread and stack and asscociates
		itself with a player instance."""

		threading.Thread.__init__( self )
		self.player = player
		self.stack = aml.thread_stack( player, local = True )
		self.player.control_start( )

	def __del__( self ):
		self.player = None
		self.stack = None

	def run( self ):
		"""Loops on stdin reading and submits all commands to the stack."""

		self.stack.include( 'shell.aml' )
		readline.parse_and_bind( 'tab: complete' )
		try:
			while self.player.active( ):
				input = raw_input( 'aml> ' ).strip( )
				# Gack! drag and drop from nautilus...
				input = input.replace( "\'", "\"" ).replace( "\"\\\"\"", "\'" )
				if input != '':
					try:
						self.player.cond.acquire( )
						self.stack.parse( input )
					except Exception, e:
						print e
					finally:
						self.player.cond.release( )
		except EOFError:
			print
		finally:
			self.player.control_end( )
			self.stack.stop_server( )
