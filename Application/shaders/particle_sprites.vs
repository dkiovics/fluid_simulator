#version 460 core
precision highp float;

layout (location = 0) in vec4 pos;
layout (location = 2) in vec2 texCoordIn;
layout (location = 10) in vec4 instanceOffset;
//layout (location = 11) in vec4 instanceColorIn;

out vec2 texCoord;
out vec3 eyeSpacePos;
//out vec4 color;
out mat3 billboardM;

uniform struct{
    mat4 viewMatrix;
	mat4 viewMatrixInverse;
	mat4 projectionMatrix;
	mat4 projectionMatrixInverse;
    vec4 position;
} camera;

uniform float particleRadius;

void main() {
	eyeSpacePos = (camera.viewMatrix * vec4(instanceOffset.xyz, 1)).xyz;
	vec3 z = normalize(eyeSpacePos);
	vec3 x = normalize(cross(vec3(0, 1, 0), z));
	vec3 y = normalize(cross(z, x));
	
	billboardM = mat3(x, y, z);
	
	vec3 vertexPos = billboardM * pos.xyz;
	
	gl_Position = camera.projectionMatrix * vec4(eyeSpacePos + vertexPos * particleRadius * 2.0, 1);
	texCoord = texCoordIn;
	//color = instanceColorIn;
	
	/*
	vec4 eyeSpacePosTmp = (camera.viewMatrix * vec4(instanceOffset.xyz, 1));
	
	eyeSpacePos = eyeSpacePosTmp.xyz;
	
	vec4 projectedQuad = camera.projectionMatrix * vec4(particleRadius, particleRadius, eyeSpacePos.z, eyeSpacePosTmp.w);
	vec3 projectedSize = vec3(projectedQuad.xy / projectedQuad.w, 1);
	
	gl_Position = camera.projectionMatrix * vec4(eyeSpacePos, 1);
	gl_Position.xyz += pos.xyz * projectedSize * 2.0 * gl_Position.w;
	texCoord = texCoordIn;
	color = instanceColorIn;
	*/
}