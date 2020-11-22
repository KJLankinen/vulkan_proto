#version 450

layout(location = 0) in VertexData
{
	vec3 color;
	vec2 texCoord;
} inData;

layout(location = 0) out vec4 fColor;

layout(set = 0, binding = 0) uniform sampler immutableSampler;
layout(set = 1, binding = 1) uniform texture2D colorTexture;

void main()
{
    fColor = texture(sampler2D(colorTexture, immutableSampler), inData.texCoord);
	//fColor = vec4(ubo.model[0]);
}