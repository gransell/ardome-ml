"""Ardome Media Library Python Wrapper

This module provides a high level wrapper to the core boost wrapper - as
such it primarily focuses around media player functionality and the AML
stack plugin.

The stack plugin is used to provide the shell and server functionality."""

import platform
import sys
import os

# Necessary hacks for linux - ensures characters are read correctly and that 
# the global c++ namespace is used when loading plugins
if platform.system( ) == 'Linux':
	import locale
	locale.setlocale( locale.LC_CTYPE, '' )
	try:
		import dl
		sys.setdlopenflags( dl.RTLD_NOW | dl.RTLD_GLOBAL )
	except:
		sys.setdlopenflags( 257 )

# Get the full path to the running instance of this file (note that if you 
# happen to be in the site-packages dir, __file__ is reported as relative - 
# hence the join here - however if the __file__ is absolute, the getcwd is 
# ignored)
full_path = os.path.join( os.getcwd( ), __file__ )

# Initialisate openpluginlib
import openpluginlib as pl
import openimagelib as il
import openmedialib as ml

from stack import stack
from player import player
from server import server
from thread_player import thread_player
from thread_stack import thread_stack
from thread_shell import thread_shell

pl.init( )
pl.init_log( )
pl.set_log_level( -1 )

def init( ):
	"""Initialise the GUI usage - not required for non-preview use."""

	if platform.system( ) == 'Darwin':
		from AppKit import NSApplicationLoad
		NSApplicationLoad( )

