echo "Starting SCons to build release and debug versions using Visual Studio 2003... "

scons.bat -j4 target=vs2003

if [ ! $? == 0 ]; then
  echo "Build failed. Terminating."
  exit 1
fi

echo "Building succeeded!"







