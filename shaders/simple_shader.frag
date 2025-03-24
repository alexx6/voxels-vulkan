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

void main() {
//	vec3 screenCoords = vec3(gl_FragCoord.xy, gl_FragCoord.z);
//
//	vec2 screenSize = vec2(1920., 1080.);
//  vec2 ndcXY = (screenCoords.xy / screenSize) * 2.0 - 1.0; 
//  float ndcZ = screenCoords.z * 2.0 - 1.0;
//  vec4 ndcCoords = vec4(ndcXY, ndcZ, 1.0);
//
//  vec4 eyeCoords = matrices.inverseProjection * ndcCoords;
//  eyeCoords /= eyeCoords.w;
//
//  vec4 worldCoords = matrices.inverseView * eyeCoords;
//
//	outColor = vec4(eyeCoords.z);
//	return;
	
//	outColor = vec4(1 / gl_FragCoord.w);
//	return;

	startPos = vec3(push.transform * vec4(fwpos, 1.0));
	rayDir = normalize(startPos - vec3(matrices.inverseView[3]));

//	outColor = vec4(startPos, 1.);
	ivec3 vPos;
	ivec3 vNormal;
//	outColor = vec4(1);
	outColor = traceVoxelBox(vPos, vNormal);
//	outColor += vec4(drawWireframe(), 0.0);
//	outColor += vec4(vec3(vNormal) / 10, 0);
}