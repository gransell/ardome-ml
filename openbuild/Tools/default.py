import SCons.Tool

def generate(env):
	"""Add default tools."""
	for t in SCons.Tool.tool_list(env['PLATFORM'], env):
		t = SCons.Tool.Tool(t, toolpath = env.toolpath)
		t(env)
		

def exists(env):
	return 1