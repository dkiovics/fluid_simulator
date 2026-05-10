#version 460 core
precision highp float;

in vec2 texCoord;

uniform sampler2D depthTexture;
uniform float smoothingKernelSize;	//in world coordinates

uniform struct CameraStruct {
    mat4 viewMatrix;
	mat4 projectionMatrix;
	mat4 projectionMatrixInverse;
    vec4 position;
} camera;

layout(std430, binding = 23) restrict writeonly buffer YRadiusBuffer {
	int yRadius[];
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

	// Column-major: see bilateral_x.frag for the rationale.
	int pixIdx = int(gl_FragCoord.x) * int(textSize.y) + int(gl_FragCoord.y);

	if(texture(depthTexture, texCoord).x == 1.0){
		if(calculateParams){
			yRadius[pixIdx] = 0;
		}
		discard;
		return;
	}

	const vec4 offsetOnScreen = camera.projectionMatrix * vec4(eyeSpacePos + vec3(0, smoothingKernelSize * 0.5, 0), 1.0);
	const float offsetOnScreenSize = offsetOnScreen.y / offsetOnScreen.w * 0.5 + 0.5 - texCoord.y;

	const float texelSize = 1.0 / textSize.y;
	int kernelSize = int(offsetOnScreenSize * textSize.y) * 2 + 1;
	if(kernelSize > 51)
		kernelSize = 51;
	if(kernelSize < 3)
		kernelSize = 3;

	const float standardDev = (kernelSize - 1) / 6.0;
	const float standardDev2 = standardDev * standardDev;
	const int r = kernelSize / 2;

	if(calculateParams){
		yRadius[pixIdx] = r;
	}

	for(int p = -r; p <= r; p++){
		vec2 coord = texCoord + vec2(0.0, texelSize) * p;
		float d = texture(depthTexture, coord).x;
		if(d < 1.0){
			float w = exp(-p*p / standardDev2 * 0.5);
			weightSum += w;
			depth += d * w;
		}
	}

    gl_FragDepth = depth / weightSum;
}
