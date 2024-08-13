#version 460

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 vColor;

layout( push_constant ) uniform constants
{	
	mat4 transform;
} PC;

struct CubeRenderData
{
	mat4 model;
	vec4 color;
};

layout(set = 0, binding = 1) readonly buffer TransformsStorageBuffer {
    CubeRenderData data[];
} transformStorageBuffer;

layout (location = 0) out vec4 outColor;

void main() {
	vec4 pos = vec4(position, 1.0);
	mat4 model = transformStorageBuffer.data[gl_InstanceIndex].model;
	vec4 color = transformStorageBuffer.data[gl_InstanceIndex].color;

	gl_Position = PC.transform * model * pos;
	outColor = color;
}