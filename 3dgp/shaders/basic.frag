#version 330

in vec4 color;
in vec4 position;

in vec3 normal;
vec3 normalNew;

out vec4 outColor;

//texture sampler
uniform sampler2D texture0;
uniform sampler2D textureNormal;
uniform bool useNormalMap;
in vec2 texCoord0;
in mat3 matrixTangent;

// Materials
uniform vec3 materialAmbient;
uniform vec3 materialDiffuse;
uniform vec3 materialSpecular;

//Fog
uniform vec3 fogColour;
in float fogFactor;


// View Matrix
uniform mat4 matrixView;

struct DIRECTIONAL
{
	vec3 direction;
	vec3 diffuse;
};
uniform DIRECTIONAL lightDir;

vec4 DirectionalLight(DIRECTIONAL light)
{
	vec4 color = vec4(0,0,0,0);
	vec3 L = normalize(mat3(matrixView) * light.direction);
	float NdotL = dot(normalNew, L);
	color += vec4(materialDiffuse * light.diffuse, 1) * max(NdotL,0);
	return color;
}

void main(void) 
{	
	
	//calculate new normal 
	if (useNormalMap){
		//new Normal Map
		normalNew = 2.0 * texture(textureNormal, texCoord0).xyz - vec3(1.0, 1.0, 1.0);
		normalNew = normalize(matrixTangent * normalNew);
	}
	else {
		normalNew = normal;
	}

	outColor = color;
	outColor += DirectionalLight(lightDir);
	outColor *= texture(texture0, texCoord0);	
	outColor = mix(vec4(fogColour,1),outColor,fogFactor);


}
