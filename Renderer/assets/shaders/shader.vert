#version 460

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;

layout( push_constant ) uniform constants
{	
	mat4 transform;
} PC;

layout (location = 0) out vec4 outColor;

void main() {
	gl_Position = PC.transform * vec4(position, 1.0);
	outColor = vec4(color, 1.0);
}