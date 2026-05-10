#version 460 core
precision highp float;

in vec2 texCoord;

uniform sampler2D depthTexture;
uniform isampler2D paramTexture;
uniform float blurScale;
uniform float blurDepthFalloff;
uniform float smoothingKernelSize;

uniform struct CameraStruct {
    mat4 viewMatrix;
	mat4 projectionMatrix;
	mat4 projectionMatrixInverse;
    vec4 position;
} camera;

layout(std430, binding = 20) restrict writeonly buffer XCountBuffer {
	uint xCount[];
};
layout(std430, binding = 21) restrict writeonly buffer XOffsetBuffer {
	uint xOffset[];
};

uniform bool calculateParams;


vec3 uvToEye(vec2 texCoord, float depth) {
	vec4 ndc = vec4(texCoord * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
	vec4 eyeSpacePos = camera.projectionMatrixInverse * ndc;
	return eyeSpacePos.xyz / eyeSpacePos.w;
}

vec3 getEyePos(sampler2D depthTexture, vec2 texCoord) {
	return uvToEye(texCoord, texture(depthTexture, texCoord).x);
}

void main() {
	const vec3 eyeSpacePos = getEyePos(depthTexture, texCoord);
	const vec2 textSize = textureSize(depthTexture, 0);
	float depth = 0.0;
	float weightSum = 0.0;
	const float curDepth = texture(depthTexture, texCoord).x;

	// Column-major: stores neighbouring Y-pixels at consecutive pixIdx values so the
	// gradient compute's Y-walk reads xCount/xOffset/xIndex contiguously.
	int pixIdx = int(gl_FragCoord.x) * int(textSize.y) + int(gl_FragCoord.y);

	if(curDepth == 1.0){
		if(calculateParams){
			// Sky pixel: still reserve 1 slot for the spray sentinel so the gradient
			// compute and sprayOverride can find a slot 0 to write to.
			xCount[pixIdx] = 1u;
			xOffset[pixIdx] = 1u;
		}
		discard;
		return;
	}

	const vec4 offsetOnScreen = camera.projectionMatrix * vec4(eyeSpacePos + vec3(smoothingKernelSize * 0.5, 0, 0), 1.0);
	const float offsetOnScreenSize = offsetOnScreen.x / offsetOnScreen.w * 0.5 + 0.5 - texCoord.x;

	const float texelSize = 1.0 / textSize.x;
	int filterRadius = int(offsetOnScreenSize * textSize.x);
	if(filterRadius > 35)
		filterRadius = 35;
	if(filterRadius < 1)
		filterRadius = 1;

	float blurScaleCorrected = blurScale / float(filterRadius) * 35.0;

	uint count = 0u;

	for(float x=-filterRadius; x<=filterRadius; x+=1.0) {
		vec2 coord = texCoord + vec2(texelSize, 0.0) * float(x);
		float sampleDepth = texture(depthTexture, coord).x;
		if(sampleDepth < 1.0){
			// spatial domain
			float r = x * blurScaleCorrected;
			float w = exp(-r*r);
			// range domain
			float r2 = (sampleDepth - curDepth) * blurDepthFalloff;
			float g = exp(-r2*r2);
			depth += sampleDepth * w * g;
			weightSum += w * g;

			if(calculateParams){
				int paramIndex = texture(paramTexture, coord).x;
				if(paramIndex >= 0){
					count++;
				}
			}
		}
	}

	if(calculateParams){
		// Reserve slot 0 as the spray sentinel: total slots = surfaceCount + 1.
		// Slot 0 stays -1 for non-spray pixels (gradient walks slots 1..count-1);
		// sprayOverride / x_fill replace it with sprayID for spray pixels (with count=1).
		xCount[pixIdx] = count + 1u;
		xOffset[pixIdx] = count + 1u;
	}

	if(weightSum == 0.0){
		discard;
		return;
	}

	gl_FragDepth = depth / weightSum;
}
