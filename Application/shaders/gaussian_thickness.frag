#version 460 core
precision highp float;

in vec2 texCoord;
out vec4 fragmentColor;

uniform sampler2D depthTexture;
uniform sampler2D thicknessTexture;
uniform float smoothingKernelSize;	//in world coordinates
uniform int axis;	//0 - x axis, 1 - y axis

uniform struct CameraStruct {
    mat4 viewMatrix;
	mat4 projectionMatrix;
	mat4 projectionMatrixInverse;
    vec4 position;
} camera;


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
	float thicknesss = 0.0;
	float weightSum = 0.0;
	
	if(texture(depthTexture, texCoord).x == 1.0){
		discard;
		return;
	}
	
	if(axis == 1){
		const vec4 offsetOnScreen = camera.projectionMatrix * vec4(eyeSpacePos + vec3(0, smoothingKernelSize * 0.5, 0), 1.0);
		const float offsetOnScreenSize = offsetOnScreen.y / offsetOnScreen.w * 0.5 + 0.5 - texCoord.y;

		const float texelSize = 1.0 / textSize.y;
		int kernelSize = int(offsetOnScreenSize * textSize.y) * 2 + 1;
		if(kernelSize > 51)
			kernelSize = 51;
		if(kernelSize < 3)
			kernelSize = 3;
			
		//const int kernelSize = 25;
		
		const float standardDev = (kernelSize - 1) / 6.0;
		const float standardDev2 = standardDev * standardDev;
		const int r = kernelSize / 2;
		for(int p = -r; p <= r; p++){
			float d = texture(thicknessTexture, texCoord + vec2(0.0, texelSize) * p).x;
			float w = exp(-p*p / standardDev2 * 0.5);
			weightSum += w;
			thicknesss += d * w;
		}
	}else{
		const vec4 offsetOnScreen = camera.projectionMatrix * vec4(eyeSpacePos + vec3(smoothingKernelSize * 0.5, 0, 0), 1.0);
		const float offsetOnScreenSize = offsetOnScreen.x / offsetOnScreen.w * 0.5 + 0.5 - texCoord.x;

		const float texelSize = 1.0 / textSize.x;
		int kernelSize = int(offsetOnScreenSize * textSize.x) * 2 + 1;
		if(kernelSize > 51)
			kernelSize = 51;
		if(kernelSize < 3)
			kernelSize = 3;
		//const int kernelSize = 25;
		
		const float standardDev = (kernelSize - 1) / 6.0;
		const float standardDev2 = standardDev * standardDev;
		const int r = kernelSize  / 2;
		for(int p = -r; p <= r; p++){
			float d = texture(thicknessTexture, texCoord + vec2(texelSize, 0.0) * p).x;
			float w = exp(-p*p / standardDev2 * 0.5);
			weightSum += w;
			thicknesss += d * w;
		}
	}
    
    fragmentColor.r = thicknesss / weightSum;
}
