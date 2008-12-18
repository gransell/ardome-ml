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

# Determine the directory path to the plugins directory
if platform.system() == 'Windows' or platform.system() == 'Microsoft':
	dir_path = os.path.join( full_path.rsplit( os.sep, 4 )[ 0 ], 'bin', 'aml-plugins' )
elif platform.system( ) == 'Darwin':
	dir_path = os.path.join( full_path.rsplit( os.sep, 4 )[ 0 ] )
else:
	dir_path = os.path.join( full_path.rsplit( os.sep, 4 )[ 0 ], 'ardome-ml', 'openmedialib', 'plugins' )

# Initialisate openpluginlib
import openpluginlib as pl
import openimagelib as il
import openmedialib as ml

from aml.stack import stack
from aml.player import player
from aml.server import server
from aml.thread_player import thread_player
from aml.thread_stack import thread_stack
from aml.thread_shell import thread_shell

pl.init( dir_path )
pl.set_log_level( -1 )

def init( ):
	"""Initialise the GUI usage - not required for non-preview use."""

	if platform.system( ) == 'Darwin':
		from AppKit import NSApplicationLoad
		NSApplicationLoad( )

