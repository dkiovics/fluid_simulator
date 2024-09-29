#version 460 core
precision highp float;

in vec2 texCoord;

uniform sampler2D depthTexture;
uniform isampler2D paramTexture;
uniform float smoothingKernelSize;	//in world coordinates

uniform struct CameraStruct {
    mat4 viewMatrix;
	mat4 projectionMatrix;
	mat4 projectionMatrixInverse;
    vec4 position;
} camera;

#define PARAM_NUM 10

struct FragmentParam {
	int paramNum;
	int paramIndexes[PARAM_NUM];
};

layout(std430, binding = 20) restrict writeonly buffer pixelParamsSSBO {
	FragmentParam pixelParams[];
};


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
	
	FragmentParam param;
	param.paramNum = 0;

	if(texture(depthTexture, texCoord).x == 1.0){
		pixelParams[int(gl_FragCoord.y) * int(textSize.x) + int(gl_FragCoord.x)] = param;
		discard;
		return;
	}
	const vec4 offsetOnScreen = camera.projectionMatrix * vec4(eyeSpacePos + vec3(smoothingKernelSize * 0.5, 0, 0), 1.0);
	const float offsetOnScreenSize = offsetOnScreen.x / offsetOnScreen.w * 0.5 + 0.5 - texCoord.x;

	const float texelSize = 1.0 / textSize.x;
	int kernelSize = int(offsetOnScreenSize * textSize.x) * 2 + 1;
	if(kernelSize > 51)
		kernelSize = 51;
	if(kernelSize < 3)
		kernelSize = 3;
		
	const float standardDev = (kernelSize - 1) / 6.0;
	const float standardDev2 = standardDev * standardDev;
	const int r = kernelSize  / 2;

	for(int p = -r; p <= r; p++){
		vec2 coord = texCoord + vec2(texelSize, 0.0) * float(p);
		float d = texture(depthTexture, coord).x;
		if(d < 1.0){
			float w = exp(-p*p / standardDev2 * 0.5);
			weightSum += w;
			depth += d * w;
		}
		int paramIndex = texture(paramTexture, coord).x;
		if(paramIndex >= 0){
			//check if the param is already in the list
			bool found = false;
			for(int j = 0; j < param.paramNum; j++){
				if(paramIndex == param.paramIndexes[j]){
					found = true;
					break;
				}
			}
			if(!found && param.paramNum < PARAM_NUM){
				param.paramIndexes[param.paramNum] = paramIndex;
				param.paramNum++;
			}
		}
	}

	pixelParams[int(gl_FragCoord.y) * int(textSize.x) + int(gl_FragCoord.x)] = param;
    
    gl_FragDepth = depth / weightSum;
}
