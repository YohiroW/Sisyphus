#version 450
#extension GL_ARB_separate_shader_objects： VK_NV_external_memory_capabilities
layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

void psMain()
{
    outColor = vec4(fragColor, 1.0f);
}