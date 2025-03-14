::call vcvars64.bat
call build.bat
pushd .\build\msc\debug\
call C:\repos\raddebugger\build\raddbg.exe vulkan_gui.exe
popd
