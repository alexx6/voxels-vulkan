#version 430
#pragma unroll

layout (location = 0) in vec3 fragColor;
layout (location = 0) out vec4 outColor;
layout (location = 1) in vec3 fwpos;
layout (location = 2) in flat vec3 vbPos;
layout (location = 3) in flat uint modelOffset;
layout (location = 4) in flat uint modelSize;
layout (location = 5) in flat uint priority;
layout (location = 6) in flat uint orientation;

uint sizeLevel = uint(log2(modelSize));

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

float maxDepth = sizeLevel * sqrt(2000 / length(vbPos + modelSize * 0.5 - vec3(matrices.inverseView[3])));

layout(binding = 1) buffer StorageBuffer {
    uint data[];
} ssbo;

mat3 orientations1[24] = mat3[](
    mat3( 1,  0,  0,  0,  1,  0,  0,  0,  1),
    mat3( 0,  0,  1,  0,  1,  0, -1,  0,  0),
    mat3(-1,  0,  0,  0,  1,  0,  0,  0, -1),
    mat3( 0,  0, -1,  0,  1,  0,  1,  0,  0),

		mat3( 1,  0,  0,  0,  1,  0,  0,  0,  1),
    mat3( 0,  0,  1,  0,  1,  0, -1,  0,  0),
    mat3(-1,  0,  0,  0,  1,  0,  0,  0, -1),
    mat3( 0,  0, -1,  0,  1,  0,  1,  0,  0),

		mat3( 1,  0,  0,  0,  1,  0,  0,  0,  1),
    mat3( 0,  0,  1,  0,  1,  0, -1,  0,  0),
    mat3(-1,  0,  0,  0,  1,  0,  0,  0, -1),
    mat3( 0,  0, -1,  0,  1,  0,  1,  0,  0),

		mat3( 1,  0,  0,  0,  1,  0,  0,  0,  1),
    mat3( 0,  0,  1,  0,  1,  0, -1,  0,  0),
    mat3(-1,  0,  0,  0,  1,  0,  0,  0, -1),
    mat3( 0,  0, -1,  0,  1,  0,  1,  0,  0),

		mat3( 1,  0,  0,  0,  1,  0,  0,  0,  1),
    mat3( 0,  0,  1,  0,  1,  0, -1,  0,  0),
    mat3(-1,  0,  0,  0,  1,  0,  0,  0, -1),
    mat3( 0,  0, -1,  0,  1,  0,  1,  0,  0),

		mat3( 1,  0,  0,  0,  1,  0,  0,  0,  1),
    mat3( 0,  0,  1,  0,  1,  0, -1,  0,  0),
    mat3(-1,  0,  0,  0,  1,  0,  0,  0, -1),
    mat3( 0,  0, -1,  0,  1,  0,  1,  0,  0)
);

mat3 orientations2[24] = mat3[](
    mat3( 1,  0,  0,  0,  1,  0,  0,  0,  1),
		mat3( 1,  0,  0,  0,  1,  0,  0,  0,  1),
    mat3( 1,  0,  0,  0,  1,  0,  0,  0,  1),
    mat3( 1,  0,  0,  0,  1,  0,  0,  0,  1),

    mat3( 1,  0,  0,  0, -1,  0,  0,  0, -1),
		mat3( 1,  0,  0,  0, -1,  0,  0,  0, -1),
    mat3( 1,  0,  0,  0, -1,  0,  0,  0, -1),
    mat3( 1,  0,  0,  0, -1,  0,  0,  0, -1),

    mat3( 0, -1,  0,  1,  0,  0,  0,  0,  1),
    mat3( 0, -1,  0,  1,  0,  0,  0,  0,  1),
    mat3( 0, -1,  0,  1,  0,  0,  0,  0,  1),
    mat3( 0, -1,  0,  1,  0,  0,  0,  0,  1),

    mat3( 0,  1,  0, -1,  0,  0,  0,  0,  1),
		mat3( 0,  1,  0, -1,  0,  0,  0,  0,  1), 
		mat3( 0,  1,  0, -1,  0,  0,  0,  0,  1), 
		mat3( 0,  1,  0, -1,  0,  0,  0,  0,  1),

		mat3( 1,  0,  0,  0,  0, -1,  0,  1,  0),
		mat3( 1,  0,  0,  0,  0, -1,  0,  1,  0),
		mat3( 1,  0,  0,  0,  0, -1,  0,  1,  0),
		mat3( 1,  0,  0,  0,  0, -1,  0,  1,  0),

		mat3( 1,  0,  0,  0,  0, 1,  0,  -1,  0),
		mat3( 1,  0,  0,  0,  0, 1,  0,  -1,  0),
		mat3( 1,  0,  0,  0,  0, 1,  0,  -1,  0),
		mat3( 1,  0,  0,  0,  0, 1,  0,  -1,  0)
);

vec3 rayDir;
vec3 cameraPos;
vec3 startPos;

//vec3 drawWireframe()
//{
//	ivec3 size = push.vbSize;
//	vec3 pos = startPos - vbPos;
//	float e = 0.2;
//
//	if (pos.x < e && pos.y < e)
//	{
//		gl_FragDepth = 0.;
//		return vec3(1.);
//	}
//	if (pos.x < e && pos.z < e)
//	{
//		gl_FragDepth = 0.;
//		return vec3(1.);
//	}
//	if (pos.y < e && pos.z < e)
//	{
//		gl_FragDepth = 0.;
//		return vec3(1.);
//	}
//	if (pos.x > size.x - e && pos.y > size.y - e)
//	{
//		gl_FragDepth = 0.;
//		return vec3(1.);
//	}
//	if (pos.x > size.x - e && pos.z > size.z - e)
//	{
//		gl_FragDepth = 0.;
//		return vec3(1.);
//	}
//	if (pos.y > size.y - e && pos.z > size.z - e)
//	{
//		gl_FragDepth = 0.;
//		return vec3(1.);
//	}
//	if (pos.x < e && pos.y > size.y - e)
//	{
//		gl_FragDepth = 0.;
//		return vec3(1.);
//	}
//	if (pos.x > size.x - e && pos.y < e)
//	{
//		gl_FragDepth = 0.;
//		return vec3(1.);
//	}
//	if (pos.x < e && pos.z > size.z - e)
//	{
//		gl_FragDepth = 0.;
//		return vec3(1.);
//	}
//	if (pos.x > size.z - e && pos.z < e)
//	{
//		gl_FragDepth = 0.;
//		return vec3(1.);
//	}
//	if (pos.y < e && pos.z > size.z - e)
//	{
//		gl_FragDepth = 0.;
//		return vec3(1.);
//	}
//	if (pos.y > size.y - e && pos.z < e)
//	{
//		gl_FragDepth = 0.;
//		return vec3(1.);
//	}
//
//	return vec3(0.);
//}

struct TracingInfo
{
	uint address;
	ivec3 octant;
	ivec3 nodePos;
};

TracingInfo tracingData[10];

//tracing state
int depth = 0;
uint nodeAddress = 0;
uint nextAxis = 0;
uint nextDir = 0;
ivec3 nodeSize = ivec3(modelSize);
ivec3 curOctant = ivec3(0);
ivec3 nodePos = ivec3(0);
vec3 treePos;
bool isLeaf = false;
vec3 invRayDir;
ivec3 vdir;
uint voxelColor = 0;
bool octantIsSet = false;
vec3 treeStartPos = vec3(0);

uint convertOctant()
{
	return curOctant.x | (curOctant.y << 1) | (curOctant.z << 2);
}

ivec3 deconvertOctant(uint octant)
{
	return ivec3(octant % 2, octant / 2 % 2, octant / 4);
}

void goToChildNode()
{
	uint localChildIndex = (ssbo.data[modelOffset + nodeAddress + 2] >> (convertOctant() * 4)) & 15;
	
	float isLocalIndexValid = float(localChildIndex != 8);

	nodeAddress = uint(mix(0, ssbo.data[modelOffset + nodeAddress + 3 + localChildIndex], isLocalIndexValid));
	isLeaf = bool(mix(1, ssbo.data[modelOffset + nodeAddress], isLocalIndexValid));
}

void stepIn()
{
	depth += 1;
	nodeSize /= 2;
	nodePos += curOctant * nodeSize;

	goToChildNode();

	tracingData[depth].address = nodeAddress;
	tracingData[depth].octant = curOctant;
	tracingData[depth].nodePos = nodePos;
}

void stepOut()
{
	depth--;

	nodeSize *= 2;
	curOctant = tracingData[depth].octant;
	nodePos = tracingData[depth].nodePos;
	nodeAddress = tracingData[depth].address;
}

void traceIn()
{
	isLeaf = false;

	if (!octantIsSet) 
	{
		curOctant = ivec3(greaterThan(treePos, nodePos + nodeSize * 0.5));
	}
	stepIn();

	while (!isLeaf && float(depth) < maxDepth)
	{
		curOctant = ivec3(greaterThan(treePos, nodePos + nodeSize * 0.5));

		stepIn();
	} 

	if (nodeAddress == 0)
	{
		voxelColor = 0;
		return;
	}

	voxelColor = ssbo.data[modelOffset + nodeAddress + 1];
}

void setNextNode()
{
	while ((((convertOctant() >> nextAxis) & 1) == nextDir) && depth > 0)
	{	
		stepOut();
	}
	
	if (depth == 0) {
		depth = -1;
		return;
	}

	stepOut();

	curOctant = tracingData[depth + 1].octant;
	
	int offset = (1 << nextAxis);

	curOctant = deconvertOctant(convertOctant() + int(mix(-offset, offset, float(nextDir > 0))));

	octantIsSet = true;
}

void stepTree()
{
	vec3 l = (nodePos + vdir * nodeSize - treePos) * invRayDir;

	bvec3 mask = lessThan(l.xxy, l.yzz);
	float useX = float(mask.x && mask.y);
	float useY = float(mask.z);

	treePos += rayDir * mix(mix(l.z, l.y, useY), l.x, useX);
	nextDir = uint(mix(mix(vdir.z, vdir.y, useY), vdir.x, useX));
	nextAxis = uint(mix(mix(2, 1, useY), 0, useX));
}

vec4 traceVoxelBoxTree()
{
	tracingData[0].address = 0;
	tracingData[0].octant = ivec3(0);
	tracingData[0].nodePos = ivec3(0);


	treePos += rayDir * 0.001;
	if (all(greaterThan(vec3(matrices.inverseView[3]) - vbPos, vec3(0))) && all(lessThan(vec3(matrices.inverseView[3]) - vbPos, vec3(modelSize))))
	{
		treePos = vec3(matrices.inverseView[3]) - vbPos;
		startPos = vec3(matrices.inverseView[3]);
	}

	treePos = orientations1[orientation % 4] * orientations2[orientation] * (treePos - vec3(modelSize * 0.5)) + vec3(modelSize * 0.5);
	rayDir = orientations1[orientation % 4] * orientations2[orientation] * rayDir;

	treeStartPos = treePos;

	vdir = ivec3(greaterThan(rayDir, vec3(0.)));


	invRayDir = 1. / rayDir;

	curOctant = ivec3(greaterThan(treePos, nodePos + nodeSize * 0.5));

	while (depth >= 0)
	{
		traceIn();

		if (voxelColor > 0)
		{
			return unpackUnorm4x8(voxelColor);
		}

		stepTree();

		setNextNode();
	}

	discard;
}

void main() {
	maxDepth = 5;
	startPos = fwpos;

	rayDir = normalize(startPos - vec3(matrices.inverseView[3]));
	treePos = startPos - vbPos;
	ivec3 vPos;
	ivec3 vNormal;

	traceVoxelBoxTree();
	float distanceToCamera = length(startPos - vec3(matrices.inverseView[3])) + length(treeStartPos - treePos);
	gl_FragDepth = max(distanceToCamera / 2000000.0, 0);
//	gl_FragDepth *= (1 - priority * 0.0000001);

	outColor = vec4(1);
	outColor.x = distanceToCamera;
}