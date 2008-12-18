echo "Starting SCons to build release and debug versions using Visual Studio 2008... "

scons.bat target=vs2008 build/debug/vs2008/bin/opencorelib_cl.dll build/debug/vs2008/bin/opencore_unit_tests.exe build/release/vs2008/bin/opencorelib_cl.exe build/release/vs2008/bin/opencore_unit_tests.exe

if [ ! $? == 0 ]; then
  echo "Build failed. Terminating."
  exit 1
fi

echo "Building succeeded!"

