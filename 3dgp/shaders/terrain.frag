#version 330

//uniforms: water
uniform vec3 waterColor;
uniform sampler2D textureBed;
uniform sampler2D textureShore;
uniform sampler2D textureSnow;

//uniforms: lighting
uniform vec3 materialAmbient;
uniform vec3 materialDiffuse;
uniform mat4 matrixView;


// Input Variables (received from Vertex Shader)
in vec4 color;
in vec4 position;
in vec3 normal;
in vec2 texCoord0;
in float waterDepth;
in float snowAltitude;
in float fogFactor;

// Output Variable (sent down through the Pipeline)
out vec4 outColor;

struct DIRECTIONAL
{	
	vec3 direction;
	vec3 diffuse;
};
uniform DIRECTIONAL lightDir;

vec4 DirectionalLight(DIRECTIONAL light)
{
	// Calculate Directional Light
	vec4 color = vec4(0, 0, 0, 0);
	vec3 L = normalize(mat3(matrixView) * light.direction);
	float NdotL = dot(normal, L);
	if (NdotL > 0)
		color += vec4(materialDiffuse * light.diffuse, 1) * NdotL;
	return color;
}


void main(void) 
{
	outColor = color;
	outColor += DirectionalLight(lightDir);

	float isAboveSnow = clamp(-snowAltitude, 0, 1);
	outColor *= mix(texture(textureSnow, texCoord0), texture(textureShore, texCoord0), isAboveSnow);

	// shoreline multitexturing
	float isAboveWater = clamp(-waterDepth, 0, 1);
	outColor *= mix(texture(textureBed, texCoord0), texture(textureShore, texCoord0), isAboveWater);
	

	outColor = mix(vec4(waterColor, 1), outColor, fogFactor);


}
