#version 460 core
#extension GL_NV_shader_atomic_float : enable
precision highp float;

in vec2 texCoord;

uniform sampler2D referenceImage;
uniform sampler2D plusPertImage;
uniform sampler2D minusPertImage;
uniform usampler2D contributionImage;

uniform float multiplier;

struct ResultSSBOData {
	float signedPerturbation[4];
	float paramPositiveOffset[4];
	float paramNegativeOffset[4];
};

layout(std430, binding = 2) restrict readonly buffer perturbationResultSSBO {
	ResultSSBOData perturbationResult[];
};

layout(std430, binding = 10) restrict writeonly buffer gradientSSBO {
	vec4 gradient[];
};

void main(void) {
	vec4 referenceColor = texture(referenceImage, texCoord);
	vec4 plusPertColor = texture(plusPertImage, texCoord);
	vec4 minusPertColor = texture(minusPertImage, texCoord);
	uint contributingParam = texture(contributionImage, texCoord).r;

	float positiveError = dot(referenceColor - plusPertColor, referenceColor - plusPertColor);
	float negativeError = dot(referenceColor - minusPertColor, referenceColor - minusPertColor);

	vec4 signedPerturbation = vec4(perturbationResult[contributingParam].signedPerturbation[0], 
		perturbationResult[contributingParam].signedPerturbation[1], perturbationResult[contributingParam].signedPerturbation[2], 
		perturbationResult[contributingParam].signedPerturbation[3]);

	if(signedPerturbation.x != 0.0)
		atomicAdd(gradient[contributingParam].x, (positiveError - negativeError) / (2 * signedPerturbation.x) * multiplier);
	if(signedPerturbation.y != 0.0)
		atomicAdd(gradient[contributingParam].y, (positiveError - negativeError) / (2 * signedPerturbation.y) * multiplier);
	if(signedPerturbation.z != 0.0)
		atomicAdd(gradient[contributingParam].z, (positiveError - negativeError) / (2 * signedPerturbation.z) * multiplier);
	if(signedPerturbation.w != 0.0)
		atomicAdd(gradient[contributingParam].w, (positiveError - negativeError) / (2 * signedPerturbation.w) * multiplier);
}