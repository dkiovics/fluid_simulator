#version 460 core
precision highp float;

layout (location = 0) in vec4 pos;
layout (location = 1) in vec4 normal;
layout (location = 2) in vec2 texCoordIn;

uniform struct {
    sampler2D colorTexture;
    mat4 modelMatrix;
	mat4 modelMatrixInverse;
    vec4 diffuseColor;
    float colorTextureScale;
    float shininess;
    vec4 specularColor;
} object;

uniform struct{
    mat4 viewMatrix;
	mat4 projectionMatrix;
    vec4 position;
} camera;

out vec4 worldPosition;
out vec4 worldNormal;
out vec2 textureCoords;

void main() {
	worldPosition = object.modelMatrix * pos;
	gl_Position = camera.projectionMatrix * camera.viewMatrix * worldPosition;
	worldNormal = object.modelMatrixInverse * normal;
	textureCoords = texCoordIn;
}