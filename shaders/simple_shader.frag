#version 430

layout (location = 0) in vec3 fragColor;
layout (location = 0) out vec4 outColor;
layout (location = 1) in vec3 fwpos;

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

vec3 rayDir;
vec3 cameraPos;
vec3 startPos;

uint getVoxel(ivec3 pos)
{
//	return 0;
	return ssbo.data[push.dataOffset + pos.x + pos.y * push.vbSize.x + pos.z * push.vbSize.x * push.vbSize.y];

//	if (pow(pos.x - push.vbSize.x / 2, 2) + pow(pos.y - push.vbSize.y / 2, 2) + pow(pos.z - push.vbSize.z / 2, 2) < pow(push.vbSize.x / 2, 2))
//	{
//		return 1.;
//	}
//
//	return 0.;
}

vec3 drawWireframe()
{
	ivec3 size = push.vbSize;
	vec3 pos = startPos - push.vbPos;
	float e = 0.2;

	if (pos.x < e && pos.y < e)
	{
		gl_FragDepth = 0.;
		return vec3(1.);
	}
	if (pos.x < e && pos.z < e)
	{
		gl_FragDepth = 0.;
		return vec3(1.);
	}
	if (pos.y < e && pos.z < e)
	{
		gl_FragDepth = 0.;
		return vec3(1.);
	}
	if (pos.x > size.x - e && pos.y > size.y - e)
	{
		gl_FragDepth = 0.;
		return vec3(1.);
	}
	if (pos.x > size.x - e && pos.z > size.z - e)
	{
		gl_FragDepth = 0.;
		return vec3(1.);
	}
	if (pos.y > size.y - e && pos.z > size.z - e)
	{
		gl_FragDepth = 0.;
		return vec3(1.);
	}
	if (pos.x < e && pos.y > size.y - e)
	{
		gl_FragDepth = 0.;
		return vec3(1.);
	}
	if (pos.x > size.x - e && pos.y < e)
	{
		gl_FragDepth = 0.;
		return vec3(1.);
	}
	if (pos.x < e && pos.z > size.z - e)
	{
		gl_FragDepth = 0.;
		return vec3(1.);
	}
	if (pos.x > size.z - e && pos.z < e)
	{
		gl_FragDepth = 0.;
		return vec3(1.);
	}
	if (pos.y < e && pos.z > size.z - e)
	{
		gl_FragDepth = 0.;
		return vec3(1.);
	}
	if (pos.y > size.y - e && pos.z < e)
	{
		gl_FragDepth = 0.;
		return vec3(1.);
	}

	return vec3(0.);
}

vec4 traceVoxelBox(out ivec3 vPos, out ivec3 vNormal)
{
	ivec3 size = push.vbSize;
//	vec3 startPos = cameraPos + gl_FragCoord.z / gl_FragCoord.w * rayDir;
	vec3 pos = startPos - push.vbPos;

	ivec3 vdir = ivec3(greaterThan(rayDir, vec3(0.)));
	ivec3 curVoxel = ivec3(floor(pos + rayDir * 0.001));

	vec3 invRayDir = 1. / rayDir;
	
	if (!all(lessThan(abs(curVoxel * 2 + ivec3(1) - push.vbSize),  vec3(push.vbSize))))
	{
		pos = vec3(matrices.inverseView[3]) - push.vbPos;
		curVoxel = ivec3(floor(pos + rayDir * 0.001));
	}

//	if(all(equal(curVoxel, ivec3(0))))
//	{
//		gl_FragDepth = 1;
//		return vec4(0);
//	}
//	any(greaterThan(pos, ivec3(0))) && all(lessThan(curVoxel, size))
//	return vec4(vec3(curVoxel) / push.vbSize, 1);

	//Used to get normals
	ivec3 lastVoxel = curVoxel;
	while (all(lessThan(abs(curVoxel * 2 + ivec3(1) - push.vbSize),  vec3(push.vbSize))))
	{
		uint voxelColor = getVoxel(curVoxel);

		if (voxelColor > 0)
		{
			vNormal = curVoxel - lastVoxel;
			vPos = curVoxel;
			
			float distanceToCamera = length(pos + push.vbPos - vec3(matrices.inverseView[3]));
			gl_FragDepth = max(distanceToCamera / 10000.0, 0);

//			return vec4((distanceToCamera - 0.1) / (1000.0 - 0.1));
			return vec4((voxelColor) & 0xFF, (voxelColor >> 8) & 0xFF, (voxelColor >> 16) & 0xFF, (voxelColor >> 24) & 0xFF) / 255.f;
		}

		lastVoxel = curVoxel;

		vec3 l = (curVoxel + vdir - pos) * invRayDir;
		if (l.x < l.y && l.x < l.z)
		{
			pos += rayDir * l.x;
			curVoxel.x += vdir.x * 2 - 1;
			continue;
		}
		else if (l.y < l.z)
		{
			pos += rayDir * l.y;
			curVoxel.y += vdir.y * 2 - 1;
			continue;
		}

		pos += rayDir * l.z;
		curVoxel.z += vdir.z * 2 - 1;
	}

	gl_FragDepth = 1;
	return vec4(0);
}

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
ivec3 nodeSize = ivec3(256);
ivec3 curOctant = ivec3(0);
ivec3 nodePos = ivec3(0);
vec3 treePos;
bool isLeaf = false;
vec3 invRayDir;
ivec3 vdir;
uint voxelColor = 0;
bool octantIsSet = false;

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
	uint localChildIndex = (ssbo.data[nodeAddress + 2] >> (convertOctant() * 4)) & 15;
	
	if (localChildIndex == 8)
	{
		nodeAddress = 0;
		isLeaf = true;
	}
	else 
	{
		nodeAddress = ssbo.data[nodeAddress + 3 + localChildIndex];
		isLeaf = bool(ssbo.data[nodeAddress]);
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

	while (!isLeaf && float(depth) < 8 * sqrt(1000 / length(push.vbPos + push.vbSize / 2 - vec3(matrices.inverseView[3]))))
	{
		curOctant = ivec3(greaterThan(treePos, nodePos + nodeSize / 2));

		stepIn();
	} 

	if (nodeAddress == 0)
	{
		voxelColor = 0;
		return;
	}

	voxelColor = ssbo.data[nodeAddress + 1];
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
	vdir = ivec3(greaterThan(rayDir, vec3(0.)));
	invRayDir = 1. / rayDir;

	tracingData[0].address = 0;
	tracingData[0].octant = ivec3(0);
	tracingData[0].nodePos = ivec3(0);


	treePos += rayDir * 0.001;
	if (all(greaterThan(vec3(matrices.inverseView[3]) - push.vbPos, vec3(0))) && all(lessThan(vec3(matrices.inverseView[3]) - push.vbPos, vec3(256))))
	{
		treePos = vec3(matrices.inverseView[3]) - push.vbPos;
	}

	int a = 100;

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
	startPos = vec3(push.transform * vec4(fwpos, 1.0));
	rayDir = normalize(startPos - vec3(matrices.inverseView[3]));
	treePos = startPos - push.vbPos;
//	outColor = vec4(startPos, 1.);
	ivec3 vPos;
	ivec3 vNormal;
	outColor = vec4(1);

	outColor = traceVoxelBoxTree();
//	outColor = vec4(vdir, 1);
	float distanceToCamera = length(treePos + push.vbPos - vec3(matrices.inverseView[3]));
//	outColor = vec4(distanceToCamera /10000);
	gl_FragDepth = max(distanceToCamera / 5000000.0, 0);
//	outColor = traceVoxelBox(vPos, vNormal);

//		outColor += vec4(drawWireframe(), 0.0);
//		outColor += vec4(0.05 * nextAxis);
}