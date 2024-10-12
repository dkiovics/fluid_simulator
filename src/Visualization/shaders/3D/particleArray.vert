#version 460 core
precision highp float;

layout (location = 0) in vec4 pos;
layout (location = 1) in vec4 normal;
layout (location = 2) in vec2 texCoordIn;

uniform struct {
    mat4 modelMatrix;
	mat4 modelMatrixInverse;
    vec4 specularColor;
    float shininess;
} object;

uniform struct{
    mat4 viewMatrix;
	mat4 projectionMatrix;
    vec4 position;
} camera;

struct ParticleShaderData
{
	vec4 posAndSpeed;
	vec4 density;
};

layout(std430, binding = 60) restrict readonly buffer particleShaderDataSSBO {
	ParticleShaderData particleShaderData[];
};

out vec4 worldPosition;
out vec4 worldNormal;
out vec2 textureCoords;
out vec4 diffuseColor;
flat out unsigned int instanceID;

uniform bool speedColorEnabled;
uniform float maxSpeedInv;
uniform vec3 color;
uniform vec3 speedColor;

void main() {
	ParticleShaderData data = particleShaderData[gl_InstanceID];
	worldPosition = (object.modelMatrix * pos) + vec4(data.posAndSpeed.xyz, 0);
	gl_Position = camera.projectionMatrix * camera.viewMatrix * worldPosition;
	worldNormal = object.modelMatrixInverse * normal;
	textureCoords = texCoordIn;
	if(speedColorEnabled)
	{
		float coeff = pow(data.posAndSpeed.w * maxSpeedInv, 0.2);
		diffuseColor = vec4(color * (1.0 - coeff) + speedColor * coeff, 1.0);
	}
	else
	{
		diffuseColor = vec4(color, 1.0);
	}
	instanceID = gl_InstanceID;
}