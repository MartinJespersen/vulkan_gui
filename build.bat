@echo off
:: Set paths and filenames
set "cwd=C:\repos\vulkan_gui\"
set "debug_dir=%cwd%build\msc\debug"
set "exec_name=vulkan_gui.exe"
set "entrypoint_file_name=entrypoint.cpp"
set "main_file_name=main.cpp"
set "exec_full_path=%debug_dir%\%exec_name%"


:: Compiler and linker flags
set "cxxflags=/W3 /std:c++20 /wd4201"
set "include_dirs=/I. /IC:\VulkanSDK\1.4.304.1\Include\ /IC:\glfw-3.4.bin.WIN64\include /IC:\freetype-2.13.2\include"
:: free typed is the debug lib
set "link_libs=glfw3_mt.lib vulkan-1.lib user32.lib gdi32.lib opengl32.lib shell32.lib freetype.lib"
set "link_flags=/ignore:4099 /MACHINE:X64"
set "link_dirs=/LIBPATH:C:\glfw-3.4.bin.WIN64\lib-vc2019\ /LIBPATH:C:\freetype-2.13.2\lib /LIBPATH:C:\VulkanSDK\1.4.304.1\Lib"


:: Compile shaders
pushd shaders
call compile.bat
popd

:: Create debug directory
if not exist "%debug_dir%" mkdir "%debug_dir%"

:: Compile main executable
cl /Z7 /DASAN_ENABLED %main_file_name% %entrypoint_file_name% /Fe"%exec_full_path%"  %cxxflags%  %include_dirs% /DPROFILING_ENABLE /nologo /link %link_dirs% %link_libs% %link_flags% /INCREMENTAL:NO /noexp
