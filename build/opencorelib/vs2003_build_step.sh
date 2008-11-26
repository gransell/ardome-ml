echo "Starting SCons to build release and debug versions using Visual Studio 2003... "

scons.bat target=vs2003 build/debug/vs2003/bin/opencorelib_cl.dll build/debug/vs2003/bin/opencore_unit_tests.exe build/release/vs2003/bin/opencorelib_cl.exe build/release/vs2003/bin/opencore_unit_tests.exe

if [ ! $? == 0 ]; then
  echo "Build failed. Terminating."
  exit 1
fi

echo "Building succeeded!"

