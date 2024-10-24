#version 460 core
precision highp float;

layout (location = 0) in vec4 pos;
layout (location = 1) in vec4 normal;
layout (location = 2) in vec2 texCoordIn;

uniform float densityThreshold;

struct ParticleGradientData {
	vec4 position;
	vec4 gradient;
};

layout(std430, binding = 8) restrict readonly buffer optimizedParticleGradientDataSSBO {
	ParticleGradientData optimizedParticleGradientData[];
};

uniform struct{
    mat4 viewMatrix;
	mat4 projectionMatrix;
    vec4 position;
} camera;

out vec4 worldPosition;
out vec4 worldNormal;
out vec4 color;

// Function to rotate a vector by a quaternion
vec3 rotateVectorByQuaternion(vec3 v, vec4 q) {
    // Quaternion multiplication to rotate vector
    return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

// Function to get quaternion for rotating from 'fromDir' to 'toDir'
vec4 getRotationQuaternion(vec3 fromDir, vec3 toDir) {
    // Normalize both vectors
    vec3 v0 = normalize(fromDir);
    vec3 v1 = normalize(toDir);

    // Compute the axis of rotation
    vec3 axis = cross(v0, v1);
    float dotProd = dot(v0, v1);

    // Handle the case when vectors are almost parallel
    if (dotProd > 0.9999) {
        // No rotation needed
        return vec4(0.0, 0.0, 0.0, 1.0);
    }

    // Handle the case when vectors are opposite
    if (dotProd < -0.9999) {
        // Rotate 180 degrees around an arbitrary perpendicular axis
        axis = normalize(cross(v0, vec3(1.0, 0.0, 0.0)));
        if (length(axis) < 0.0001) {
            axis = normalize(cross(v0, vec3(0.0, 0.0, 1.0)));
        }
        return vec4(axis, 0.0);
    }

    // Compute quaternion
    float angle = acos(dotProd);
    return vec4(normalize(axis) * sin(angle / 2.0), cos(angle / 2.0));
}

void main() {
	ParticleGradientData data = optimizedParticleGradientData[gl_InstanceID];

    if(data.gradient.w > densityThreshold) {
		gl_Position = vec4(10000.0, 10000.0, 10000.0, 1.0);
        return;
	}
    vec3 normalizedGradient = normalize(data.gradient.xyz);
	vec4 rotationQuat = getRotationQuaternion(vec3(0.0, 1.0, 0.0), normalizedGradient);

    vec3 transformedVertex = rotateVectorByQuaternion(pos.xyz, rotationQuat);

	gl_Position = camera.projectionMatrix * camera.viewMatrix * vec4(data.position.xyz + transformedVertex, 1.0);
	worldNormal = vec4(rotateVectorByQuaternion(normal.xyz, rotationQuat), 0.0);
	color = vec4(normalizedGradient, 1.0);
}