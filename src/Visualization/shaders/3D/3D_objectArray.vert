#version 460 core
precision highp float;

layout (location = 0) in vec4 pos;
layout (location = 1) in vec4 normal;
layout (location = 2) in vec2 texCoordIn;
layout (location = 10) in vec4 instanceOffset;
layout (location = 11) in vec4 instanceColor;

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

out vec4 worldPosition;
out vec4 worldNormal;
out vec2 textureCoords;
out vec4 diffuseColor;
flat out unsigned int instanceID;

void main() {
	worldPosition = (object.modelMatrix * pos) + instanceOffset;
	gl_Position = camera.projectionMatrix * camera.viewMatrix * worldPosition;
	worldNormal = object.modelMatrixInverse * normal;
	textureCoords = texCoordIn;
	diffuseColor = instanceColor;
	instanceID = gl_InstanceID;
}