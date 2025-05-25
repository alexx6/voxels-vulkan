#version 430

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

TracingInfo tracingData[21];

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
	return curOctant.x + curOctant.y * 2 + curOctant.z * 4;
}

ivec3 deconvertOctant(uint octant)
{
	return ivec3(octant % 2, octant / 2 % 2, octant / 4);
}

void goToChildNode()
{
	uint localChildIndex = (ssbo.data[modelOffset + nodeAddress + 2] >> (convertOctant() * 4)) & 15;
	
	if (localChildIndex == 8)
	{
		nodeAddress = 0;
		isLeaf = true;
	}
	else 
	{
		nodeAddress = ssbo.data[modelOffset + nodeAddress + 3 + localChildIndex];
		isLeaf = bool(ssbo.data[modelOffset + nodeAddress]);
	}
}

void stepIn()
{
	//update state
	depth += 1;
	nodeSize /= 2;
	nodePos += curOctant * nodeSize;

	//update node address
	goToChildNode();

	//update data
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
		curOctant = ivec3(greaterThan(treePos, nodePos + nodeSize / 2));
	}
	stepIn();

	while (!isLeaf && float(depth) < sizeLevel * sqrt(2000 / length(vbPos + push.vbSize / 2 - vec3(matrices.inverseView[3]))))
	{
		curOctant = ivec3(greaterThan(treePos, nodePos + nodeSize / 2));

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

	if (nextDir > 0) 
	{
			curOctant = deconvertOctant(convertOctant() + (1 << nextAxis));
	}
	else
	{
			curOctant = deconvertOctant(convertOctant() - (1 << nextAxis));
	}

	octantIsSet = true;
}

void stepTree()
{
	vec3 l = (nodePos + vdir * nodeSize - treePos) * invRayDir;

	if (l.x < l.y && l.x < l.z)
	{
		treePos += rayDir * l.x;
		nextDir = vdir.x;
		nextAxis = 0;
	}
	else if (l.y < l.z)
	{
		treePos += rayDir * l.y;
		nextDir = vdir.y;
		nextAxis = 1;
	}
	else
	{
		treePos += rayDir * l.z;
		nextDir = vdir.z;
		nextAxis = 2;
	} 
}

vec4 convertColor()
{
	return vec4((voxelColor) & 0xFF, (voxelColor >> 8) & 0xFF, (voxelColor >> 16) & 0xFF, (voxelColor >> 24) & 0xFF) / 255.f;
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

	treePos = orientations1[orientation % 4] * orientations2[orientation] * (treePos - vec3(modelSize / 2)) + vec3(modelSize / 2);
	rayDir = orientations1[orientation % 4] * orientations2[orientation] * rayDir;

	treeStartPos = treePos;

//	treePos = orientations1[orientation % 4] * (treePos - vec3(modelSize / 2)) + vec3(modelSize / 2);
//	rayDir = orientations1[orientation % 4] * rayDir;

//	treePos = vec3(modelSize - treePos.y, treePos.x, treePos.z);
//	rayDir = vec3(-rayDir.y, rayDir.x, rayDir.z);
	vdir = ivec3(greaterThan(rayDir, vec3(0.)));


	invRayDir = 1. / rayDir;

//	stepIn();
	curOctant = ivec3(greaterThan(treePos, nodePos + nodeSize / 2));

	while (depth >= 0)
	{
		traceIn();

		if (voxelColor > 0)
		{
			return convertColor();
		}

		stepTree();

		setNextNode();
	}

	discard;
}

void main() {
//	outColor = vec4(fragColor, 1);
//	gl_FragDepth = length(fwpos - vec3(matrices.inverseView[3])) / 2000000.0;
//	return;

//	startPos = vec3(push.transform * vec4(fwpos, 1.0));
		startPos = fwpos;

	rayDir = normalize(startPos - vec3(matrices.inverseView[3]));
	treePos = startPos - vbPos;
//	outColor = vec4(startPos, 1.);
	ivec3 vPos;
	ivec3 vNormal;
//	outColor = vec4(1);

//	traceVoxelBoxTree();
	outColor = traceVoxelBoxTree();
//	outColor = vec4(vdir, 1);
	float distanceToCamera = length(startPos - vec3(matrices.inverseView[3])) + length(treeStartPos - treePos);
//	outColor = vec4(distanceToCamera /10000);
	gl_FragDepth = max(distanceToCamera / 2000000.0, 0);
	gl_FragDepth *= (1 - priority * 0.0000001);
//	outColor = vec4(modelOffset);
//	gl_FragDepth = gl_FragCoord.w;
//	outColor = traceVoxelBox(vPos, vNormal);

//		outColor += vec4(drawWireframe(), 0.0);
//		outColor += vec4(0.05 * nextAxis);
}