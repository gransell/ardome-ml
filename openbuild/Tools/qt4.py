"""openbuild.Tool.qt"""

import os.path
import re

import SCons.Action
import SCons.Builder
import SCons.Defaults
import SCons.Scanner
import SCons.Tool
import SCons.Util

class ToolQtWarning(SCons.Warnings.Warning):
	pass

class GeneratedMocFileNotIncluded(ToolQtWarning):
	pass

class QtdirNotFound(ToolQtWarning):
	pass

SCons.Warnings.enableWarningClass(ToolQtWarning)

def detect_qt4(env):
	"""Not really safe, but fast method to detect the QT library"""
	
	try: return env['QTDIR']
	except KeyError: pass

	if env['PLATFORM'] == 'win32' : 
		for r, d, f in os.walk( 'bcomp' ):
			if 'moc.exe' in f : 
				# Want vs2003 subfolder for target vs2003, vs2005 for 2005 etc.
				if env['target'] in r.split(os.sep) :
					qtpath = os.path.join( env.root, r[ : r.rfind(os.sep)] )
					return qtpath
	else:
		for r, d, f in os.walk( 'bcomp' ):
			if 'moc' in f :
				qtpath = os.path.join( env.root, r[ : r.rfind(os.sep)] )
				print 'Found QT at path=', qtpath
				return qtpath
					
	moc = env.WhereIs('moc-qt4') or env.WhereIs('moc4') or env.WhereIs('moc')
	if moc:
		QTDIR = os.path.dirname(os.path.dirname(moc))
		SCons.Warnings.warn(
			QtdirNotFound,
			"QTDIR variable is not defined, using moc executable as a hint (QTDIR=%s)" % QTDIR)
		return QTDIR
		
	raise SCons.Errors.StopError( QtdirNotFound, "Could not detect Qt 4 installation")
	return None

def locate_qt4_command(env, command, qtdir) :
	suffixes = [ '-qt4', '-qt4.exe', '4', '4.exe', '', '.exe' ]
	triedPaths = []
	
	for suffix in suffixes :
		fullpath = os.path.join(qtdir,'bin',command + suffix)
		if os.access(fullpath, os.X_OK) :
			return fullpath
		triedPaths.append(fullpath)

	fullpath = env.Detect([command + '-qt4', command + '4', command])
	if not (fullpath is None) : return fullpath

	raise Exception, "Qt4 command '" + command + "' not found. Tried: " + ', '.join(triedPaths)

def generate( env ) :

	env['QTDIR']  = detect_qt4(env)
	env['QT4_MOC'] = locate_qt4_command(env, 'moc', env['QTDIR'])
	env['QT4_UIC'] = locate_qt4_command(env, 'uic', env['QTDIR'])
	env['QT4_RCC'] = locate_qt4_command(env, 'rcc', env['QTDIR'])
	if env['PLATFORM'] == 'win32':
		env['QT4_IDC'] = locate_qt4_command(env, 'idc', env['QTDIR'])

def exists(env):
	return detect_qt4(env)

