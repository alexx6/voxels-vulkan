#version 430

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout (location = 1) out vec3 fwpos;
layout (location = 2) out flat vec3 vbPos;
layout (location = 3) out flat uint modelOffset;
layout (location = 4) out flat uint modelSize;
layout (location = 5) out flat uint priority;
layout (location = 6) out flat uint orientation;

layout(location = 0) out vec3 fragColor;

layout(binding = 3, r32ui) uniform readonly uimage2D storageTexture1;

layout(push_constant) uniform Push {
	mat4 transform;
	mat4 projectionView;
	uint dataOffset;
	ivec3 vbPos;
	ivec3 vbSize;
} push;

layout(set = 0, binding = 0) uniform Matrices {
    mat4 view;
    mat4 inverseView;
		mat4 inverseProjection;
} matrices;

struct VoxelData
{
	ivec3 pos;
	uint size;
	uint orientation;
	uint modelOffset;
};

layout(binding = 2) buffer StorageBuffer {
    VoxelData vd[];
} ssbo;

void main() {
//	gl_Position = vec4(push.transform * position + push.offset, 0.0, 1.0); 

	mat4 newTransform = push.transform;
//
	newTransform[0] *= ssbo.vd[gl_InstanceIndex].size;
	newTransform[1] *= ssbo.vd[gl_InstanceIndex].size;
	newTransform[2] *= ssbo.vd[gl_InstanceIndex].size;

	newTransform[3].xyz += ssbo.vd[gl_InstanceIndex].pos;

	gl_Position = push.projectionView * newTransform * vec4(position, 1.0);
	
	fwpos = vec3(newTransform * vec4(position, 1.0));
	vbPos = ssbo.vd[gl_InstanceIndex].pos;
	modelOffset = ssbo.vd[gl_InstanceIndex].modelOffset;
	modelSize = ssbo.vd[gl_InstanceIndex].size;
	orientation = ssbo.vd[gl_InstanceIndex].orientation;
//
//	if (length(vbPos - vec3(matrices.inverseView[3])) > imageLoad(storageTexture1, ivec2((gl_Position.xy / gl_Position.w * 0.5 + 0.5) * vec2(1920, 1080))).x + 1024)
//	{
//		gl_Position = vec4(0.0, 0.0, 2.0, 1.0);
//	}
//
	priority = gl_InstanceIndex;
//	gl_Position += vec4(ssbo.vd[gl_InstanceIndex].pos, 0);
	fragColor = vec3(ssbo.vd[gl_InstanceIndex].modelOffset);
}