#version 460 core
precision highp float;

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoordIn;
layout (location = 10) in vec3 offset;
layout (location = 11) in vec4 color;

uniform struct{
	mat4 modelMatrix;
} object;

uniform struct{
    mat4 viewMatrix;
	mat4 projectionMatrix;
    vec3 position;
} camera;

out vec3 worldPosition;
out vec3 worldNormal;
out vec2 textureCoords;
out vec4 diffuseColor;

void main() {
	vec4 worldPositionTmp = (object.modelMatrix * vec4(pos, 1.0)) + vec4(offset, 0.0);
	worldPosition = worldPositionTmp.xyz;
	gl_Position = camera.projectionMatrix * camera.viewMatrix * worldPositionTmp;
	worldNormal = mat3(transpose(inverse(object.modelMatrix))) * normal;
	textureCoords = texCoordIn;
	diffuseColor = color;
}