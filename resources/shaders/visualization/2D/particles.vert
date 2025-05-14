#version 460 core

layout (location = 0) in vec4 pos;

uniform struct {
	mat4 modelMatrix;
	vec4 diffuseColor;
} object;

uniform struct {
	mat4 VP;
} camera;

uniform struct {
	bool speedColorEnabled;
	float maxSpeed;
	float exponent;
	vec4 maxColor;
} coloring;

struct ParticleShaderData
{
	vec4 posAndDensity;
	vec4 velocity;
};

layout(std430, binding = 1) restrict readonly buffer particleShaderDataSSBO {
	ParticleShaderData particleShaderData[];
};

out vec4 ambientColor;

void main() {
	ParticleShaderData particle = particleShaderData[gl_InstanceID];

	gl_Position = camera.VP * (object.modelMatrix * pos + vec4(particle.posAndDensity.xy, 0.0, 0.0));

	ambientColor = vec4(0.8, 0.8, 0.8, 1.0);

	if (coloring.speedColorEnabled) {
		float speed = length(particle.velocity.xyz);
		float speedFactor = (coloring.maxSpeed - speed) / coloring.maxSpeed;
		speedFactor = clamp(speedFactor, 0.0, 1.0);
		speedFactor = pow(speedFactor, coloring.exponent);
		ambientColor = mix(coloring.maxColor, object.diffuseColor, speedFactor);
	} else {
		ambientColor = object.diffuseColor;
	}
}