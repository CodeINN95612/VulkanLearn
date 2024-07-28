#version 460

const vec3 positions[] = {
	vec3( 0.0f, -0.5f, 0.0f),
	vec3( 0.5f,  0.5f, 0.0f),
	vec3(-0.5f,  0.5f, 0.0f)
};

const vec3 colors[] = {
	vec3(1.0f, 0.0f, 0.0f),
	vec3(0.0f, 1.0f, 0.0f),
	vec3(0.0f, 0.0f, 1.0f)
};

layout (location = 0) out vec4 outColor;

void main() {
	gl_Position = vec4(positions[gl_VertexIndex], 1.0);
	outColor = vec4(colors[gl_VertexIndex], 1.0);
}