C:/VulkanSDK/1.3.283.0/Bin/glslc.exe shader.vert -o vert.spv
C:/VulkanSDK/1.3.283.0/Bin/glslc.exe shader.frag -o frag.spv
cp frag.spv ../build/Release/shaders/ && cp vert.spv ../build/Release/shaders/
cp frag.spv ../build/shaders/ && cp vert.spv ../build/shaders/
cp frag.spv ../build/shaders/ && cp vert.spv ../build/shaders/
pause
