::call vcvars64.bat
call build.bat
pushd .\msc\build\debug\
call C:\repos\raddebugger\build\raddbg.exe vulkan_gui.exe
popd
