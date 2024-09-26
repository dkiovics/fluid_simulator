#version 460 core
#extension GL_NV_shader_atomic_float : enable
precision highp float;

in vec2 texCoord;

uniform sampler2D referenceImage;
uniform sampler2D plusPertImage;
uniform sampler2D minusPertImage;

uniform float multiplier;
uniform ivec2 screenSize;

struct ResultSSBOData {
	float signedPerturbation[4];
	float paramPositiveOffset[4];
	float paramNegativeOffset[4];
};

struct FragmentParam {
	int paramNum;
	int paramIndexes[40];
};

layout(std430, binding = 2) restrict readonly buffer perturbationResultSSBO {
	ResultSSBOData perturbationResult[];
};

layout(std430, binding = 10) restrict writeonly buffer gradientSSBO {
	vec4 gradient[];
};

layout(std430, binding = 30) restrict readonly buffer pixelParamsSSBO {
	FragmentParam pixelParams[];
};

void main(void) {
	vec4 referenceColor = texture(referenceImage, texCoord);
	vec4 plusPertColor = texture(plusPertImage, texCoord);
	vec4 minusPertColor = texture(minusPertImage, texCoord);
	float positiveError = dot(referenceColor - plusPertColor, referenceColor - plusPertColor);
	float negativeError = dot(referenceColor - minusPertColor, referenceColor - minusPertColor);

	int index = int(gl_FragCoord.y) * screenSize.x + int(gl_FragCoord.x);
	FragmentParam contributingParam = pixelParams[index];

	for(int p = 0; p < contributingParam.paramNum; p++)
	{
		int contributingParamIndex = contributingParam.paramIndexes[p];
		vec4 signedPerturbation = vec4(perturbationResult[contributingParamIndex].signedPerturbation[0], 
			perturbationResult[contributingParamIndex].signedPerturbation[1], perturbationResult[contributingParamIndex].signedPerturbation[2], 
			perturbationResult[contributingParamIndex].signedPerturbation[3]);

		if(signedPerturbation.x != 0.0)
			atomicAdd(gradient[contributingParamIndex].x, (positiveError - negativeError) / (2 * signedPerturbation.x) * multiplier);
		if(signedPerturbation.y != 0.0)
			atomicAdd(gradient[contributingParamIndex].y, (positiveError - negativeError) / (2 * signedPerturbation.y) * multiplier);
		if(signedPerturbation.z != 0.0)
			atomicAdd(gradient[contributingParamIndex].z, (positiveError - negativeError) / (2 * signedPerturbation.z) * multiplier);
		if(signedPerturbation.w != 0.0)
			atomicAdd(gradient[contributingParamIndex].w, (positiveError - negativeError) / (2 * signedPerturbation.w) * multiplier);
	}
}