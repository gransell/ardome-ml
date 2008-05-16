echo "Starting Visual Studio .NET 2008 to build release version... "
echo -n > aml.vc9.sln.release.build.log
"${PROGRAMFILES}/Microsoft Visual Studio 9.0/Common7/IDE/devenv.exe" bcomp/ardome-ml/vs2008/ardome_media_library_vc9.sln /rebuild release /out aml.vc9.sln.release.build.log 

if [ ! $? == 0 ]; then
  cat aml.vc9.sln.release.build.log
  echo "Release build failed. Terminating."
  exit 1
fi

echo "Starting Visual Studio .NET 2008 to build debug version... "
echo -n > aml.vc9.sln.debug.build.log
"${PROGRAMFILES}/Microsoft Visual Studio 9.0/Common7/IDE/devenv.exe" bcomp/ardome-ml/vs2008/ardome_media_library_vc9.sln /rebuild debug /out aml.vc9.sln.debug.build.log

if [ ! $? == 0 ]; then
  cat aml.vc9.sln.debug.build.log
  echo "Debug build failed. Terminating."
  exit 1
fi	

echo "Building succeeded!"






