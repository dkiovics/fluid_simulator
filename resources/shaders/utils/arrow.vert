#version 460 core
precision highp float;

layout (location = 0) in vec4 pos;
layout (location = 1) in vec4 normal;
layout (location = 2) in vec2 texCoordIn;

uniform float densityThreshold;

struct ParticleSSBOData {
    vec4 posAndDensity;
    vec4 velocity;
};

// Particle positions (and density in posAndDensity.w).
layout(std430, binding = 0) restrict readonly buffer ParticlesSSBO {
    ParticleSSBOData particles[];
};

// Per-particle position-gradient (xyz). Same struct layout as particles so the
// stride matches and binding semantics stay symmetric.
layout(std430, binding = 1) restrict readonly buffer GradientSSBO {
    ParticleSSBOData gradient[];
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
    return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

// Function to get quaternion for rotating from 'fromDir' to 'toDir'
vec4 getRotationQuaternion(vec3 fromDir, vec3 toDir) {
    vec3 v0 = normalize(fromDir);
    vec3 v1 = normalize(toDir);

    vec3 axis = cross(v0, v1);
    float dotProd = dot(v0, v1);

    if (dotProd > 0.9999) {
        return vec4(0.0, 0.0, 0.0, 1.0);
    }

    if (dotProd < -0.9999) {
        axis = normalize(cross(v0, vec3(1.0, 0.0, 0.0)));
        if (length(axis) < 0.0001) {
            axis = normalize(cross(v0, vec3(0.0, 0.0, 1.0)));
        }
        return vec4(axis, 0.0);
    }

    float angle = acos(dotProd);
    return vec4(normalize(axis) * sin(angle / 2.0), cos(angle / 2.0));
}

void main() {
    vec4 particlePos = particles[gl_InstanceID].posAndDensity;
    vec3 particleGradient = gradient[gl_InstanceID].posAndDensity.xyz;

    // Skip arrows for particles whose density is above the threshold (the user
    // typically wants to see only edge/surface particles, not the bulk interior).
    if (particlePos.w > densityThreshold) {
        gl_Position = vec4(10000.0, 10000.0, 10000.0, 1.0);
        return;
    }

    vec3 normalizedGradient = normalize(particleGradient);
    vec4 rotationQuat = getRotationQuaternion(vec3(0.0, 1.0, 0.0), -normalizedGradient);

    vec3 transformedVertex = rotateVectorByQuaternion(pos.xyz, rotationQuat);

    worldPosition = vec4(particlePos.xyz + transformedVertex, 1.0);

    gl_Position = camera.projectionMatrix * camera.viewMatrix * worldPosition;
    worldNormal = vec4(rotateVectorByQuaternion(normal.xyz, rotationQuat), 0.0);
    color = vec4(normalizedGradient, 1.0);
}
