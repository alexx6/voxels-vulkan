#version 430

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout (location = 1) out vec3 fwpos;

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform Push {
	mat4 transform;
	mat4 projectionView;
	vec3 color;
	ivec3 vbPos;
	ivec3 vbSize;
} push;

void main() {
//	gl_Position = vec4(push.transform * position + push.offset, 0.0, 1.0); 

	fwpos = position;
	gl_Position = push.projectionView * push.transform * vec4(position, 1.0);

	fragColor = color;
}