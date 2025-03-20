#version 430

layout (location = 0) in vec3 fragColor;
layout (location = 0) out vec4 outColor;
layout (location = 1) in vec3 fwpos;

layout(push_constant) uniform Push {
	mat4 transform;
	mat4 projectionView;
	vec3 color;
	ivec3 vbPos;
	ivec3 vbSize;
} push;

layout(set = 0, binding = 0) uniform Matrices {
    mat4 view;
    mat4 inverseView;
		mat4 inverseProjection;
} matrices;

vec3 rayDir;
vec3 cameraPos;
vec3 startPos;

float getVoxel(ivec3 pos)
{
	if (pos.x + pos.y - pos.z <= 0)
	{
		return 1.;
	}

	return 0.;
}

vec4 traceVoxelBox(out ivec3 vPos, out ivec3 vNormal)
{
	ivec3 size = push.vbSize;
//	vec3 startPos = cameraPos + gl_FragCoord.z / gl_FragCoord.w * rayDir;
	vec3 pos = (startPos - push.vbPos) * 10;

	ivec3 vdir = ivec3(greaterThan(rayDir, vec3(0.)));
	ivec3 curVoxel = ivec3(floor(pos + rayDir * 0.1));

	vec3 invRayDir = 1. / rayDir;
	
	if (!all(lessThan(abs(curVoxel * 2 + ivec3(1) - push.vbSize),  vec3(push.vbSize))))
	{
		pos = (vec3(matrices.inverseView[3]) - push.vbPos + rayDir * 0.3) * 10;
		curVoxel = ivec3(floor(pos));
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
		if (getVoxel(curVoxel) > 0.)
		{
			vNormal = curVoxel - lastVoxel;
			vPos = curVoxel;
			
			float distanceToCamera = length(pos / 10 + push.vbPos - vec3(matrices.inverseView[3]));
			gl_FragDepth = max((distanceToCamera - 0.1) / (100.0 - 0.1), 0);

//			return vec4((distanceToCamera - 0.1) / (1000.0 - 0.1));
			return vec4(vec3(curVoxel) / push.vbSize, 1.0);
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
	outColor = traceVoxelBox(vPos, vNormal);
}