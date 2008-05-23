import env
import os
import vs

class VsBuilder :
	def __init__( self, vsver ) :
		self.vs_version = vsver
		
	def shared_library( self, env, lib, sources, headers=None, pre=None, nopre=None, *keywords ):
		print "Running VsBuild.shared_library"
		return []

	def plugin( self, env, lib, sources, headers=None, pre=None, nopre=None, *keywords ):
		return []	
		

