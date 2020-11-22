#version 450

layout(set = 1, binding = 0) uniform UniformBufferObject
{
	mat4 mvp;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out VertexData
{
	vec3 color;
	vec2 texCoord;
} outData;

void main()
{
    gl_Position = ubo.mvp * vec4(inPosition, 1.0);
	outData.color = inColor;
	outData.texCoord = inTexCoord;
}