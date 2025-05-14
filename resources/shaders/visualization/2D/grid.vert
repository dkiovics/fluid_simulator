#version 460 core

layout (location = 0) in vec4 pos;

uniform struct {
	mat4 VP;
} camera;

uniform vec3 increment;

uniform struct {
	vec2 simSize;
	vec2 simCenter;
} object;

out vec4 ambientColor;

void main() {
	vec4 offset = vec4(increment * float(gl_InstanceID), 0.0);
	vec2 tmpPos = pos.xy * object.simSize + object.simCenter * abs(pos.xy) * 2.0;
	gl_Position = camera.VP * (vec4(tmpPos, 0.0, 1.0) + offset);
	
	ambientColor = vec4(1.0, 0.0, 0.0, 1.0);
}