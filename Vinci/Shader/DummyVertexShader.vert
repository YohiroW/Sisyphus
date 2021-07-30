#version 450
#extension GL_ARB_separate_shader_objects： VK_NV_external_memory_capabilities
layout(location=0) out vec3 fragColor;

out gl_PerVertex
{
    vec4 gl_Position;
};

vec2 position[3] = vec2[](
    vec2(0.0f, -0.5f),
    vec2(0.5f, 0.5f),
    vec2(-0.5f, 0.5f)
);

vec3 vertexColors[3] = vec3[](
    vec3(1.0f, 0.0f, 0.0f),
    vec3(0.0f, 1.0f, 0.0f),
    vec3(0.0f, 0.0f, 1.0f)
);
void vsMain()
{
    gl_Position = vec4(positions[gl_VertexIndex], 0.0f, 1.0f);
    fragColor = vertexColors[gl_VertexIndex];
}

