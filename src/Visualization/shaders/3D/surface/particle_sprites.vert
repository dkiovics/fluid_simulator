#version 460 core
precision highp float;

layout (location = 0) in vec4 pos;
layout (location = 2) in vec2 texCoordIn;

out vec2 texCoord;
out vec3 eyeSpacePos;
out mat3 billboardM;
flat out unsigned int instanceID;
flat out float density;

uniform struct{
    mat4 viewMatrix;
	mat4 viewMatrixInverse;
	mat4 projectionMatrix;
	mat4 projectionMatrixInverse;
    vec4 position;
} camera;

struct ParticleShaderData
{
	vec4 posAndSpeed;
	vec4 density;
};

layout(std430, binding = 80) restrict readonly buffer particleShaderDataSSBO {
	ParticleShaderData particleShaderData[];
};

uniform float particleRadius;

void main() {
	density = particleShaderData[gl_InstanceID].density.x;
	eyeSpacePos = (camera.viewMatrix * vec4(particleShaderData[gl_InstanceID].posAndSpeed.xyz, 1)).xyz;
	vec3 z = normalize(eyeSpacePos);
	vec3 x = normalize(cross(vec3(0, 1, 0), z));
	vec3 y = normalize(cross(z, x));
	
	billboardM = mat3(x, y, z);
	
	vec3 vertexPos = billboardM * pos.xyz;
	
	gl_Position = camera.projectionMatrix * vec4(eyeSpacePos + vertexPos * particleRadius * 2.0, 1);
	texCoord = texCoordIn;

	instanceID = gl_InstanceID;
}