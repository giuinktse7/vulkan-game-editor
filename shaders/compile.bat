D:/Programs/VulkanSDK/1.2.141.0/Bin32/glslc.exe shader.vert -o vert.spv
D:/Programs/VulkanSDK/1.2.141.0/Bin32/glslc.exe shader.frag -o frag.spv
cp frag.spv ../build/Release/shaders/ && cp vert.spv ../build/Release/shaders/
cp frag.spv ../build/Debug/shaders/ && cp vert.spv ../build/Debug/shaders/
pause

