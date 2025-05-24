#version 430

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout (location = 1) out vec3 fwpos;
layout (location = 2) out flat vec3 vbPos;
layout (location = 3) out flat uint modelOffset;
layout (location = 4) out flat uint modelSize;
layout (location = 5) out flat uint priority;

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform Push {
	mat4 transform;
	mat4 projectionView;
	uint dataOffset;
	ivec3 vbPos;
	ivec3 vbSize;
} push;

struct VoxelData
{
	ivec3 pos;
	ivec3 size;
	uint modelOffset;
};

layout(binding = 2) buffer StorageBuffer {
    VoxelData vd[];
} ssbo;

void main() {
//	gl_Position = vec4(push.transform * position + push.offset, 0.0, 1.0); 

	mat4 newTransform = push.transform;
//
	newTransform[0] *= ssbo.vd[gl_InstanceIndex].size.x;
	newTransform[1] *= ssbo.vd[gl_InstanceIndex].size.x;
	newTransform[2] *= ssbo.vd[gl_InstanceIndex].size.x;

//	newTransform[0] *= 256;
//	newTransform[1] *= 256;
//	newTransform[2] *= 256;
	newTransform[3].xyz += ssbo.vd[gl_InstanceIndex].pos;

	gl_Position = push.projectionView * newTransform * vec4(position, 1.0);

	fwpos = vec3(newTransform * vec4(position, 1.0));
	vbPos = ssbo.vd[gl_InstanceIndex].pos;
	modelOffset = ssbo.vd[gl_InstanceIndex].modelOffset;
	modelSize = ssbo.vd[gl_InstanceIndex].size.x;
	priority = gl_InstanceIndex;
//	gl_Position += vec4(ssbo.vd[gl_InstanceIndex].pos, 0);
	fragColor = vec3(ssbo.vd[gl_InstanceIndex].modelOffset);
}