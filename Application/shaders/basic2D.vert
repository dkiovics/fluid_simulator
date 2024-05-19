#version 460 core

layout (location = 0) in vec4 pos;

uniform struct {
	mat4 modelMatrix;
	vec4 diffuseColor;
} object;

uniform struct {
	mat4 VP;
} camera;

out vec4 ambientColor;

void main() {
	gl_Position = camera.VP * object.modelMatrix * pos;
	ambientColor = object.diffuseColor;
}