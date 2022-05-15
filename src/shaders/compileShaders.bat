rem %VULKAN_SDK%/Bin/glslangValidator.exe -V shader.vert
rem %VULKAN_SDK%/Bin/glslangValidator.exe -V shader.frag
forfiles /s /m *.vert /c "cmd /c %VULKAN_SDK%/Bin/glslangValidator.exe -V @path -o ../spv/@fname.vert.spv"
forfiles /s /m *.frag /c "cmd /c %VULKAN_SDK%/Bin/glslangValidator.exe -V @path -o ../spv/@fname.frag.spv"
pause