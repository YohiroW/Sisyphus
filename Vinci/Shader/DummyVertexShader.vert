#version 450
#extension GL_ARB_separate_shader_objects: enable

layout(location= 0) in vec2 inPosition;
layout(location= 1) in vec3 inColor;
layout(location= 2) in vec2 inUV;

layout(location= 0) out vec3 fragColor;
layout(location= 1) out vec2 fragUV;

layout(binding = 0) uniform UniformBufferObject
{ 
	mat4 model;
	mat4 view;
	mat4 proj;
};

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
    gl_Position = proj* view* model* vec4(inPosition, 0.0f, 1.0f);
    fragColor = inColor;
    fragUV = inUV;
}

