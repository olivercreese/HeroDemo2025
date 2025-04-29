#version 330

// Uniforms: Transformation Matrices
uniform mat4 matrixProjection;
uniform mat4 matrixView;
uniform mat4 matrixModelView;

// Uniforms: Material Colours
uniform vec3 materialAmbient;
uniform vec3 materialDiffuse;

//Uniforms: water 
uniform float waterLevel;
uniform float snowLevel;
uniform float fogDensity;

in vec3 aVertex;
in vec3 aNormal;
in vec2 aTexCoord;

out vec4 color;
out vec4 position;
out vec3 normal;
out vec2 texCoord0;
out float waterDepth;
out float snowAltitude;
out float fogFactor;

uniform vec4 planeClip;


// Light declarations
struct AMBIENT
{	
	vec3 color;
};
uniform AMBIENT lightAmbient;


vec4 AmbientLight(AMBIENT light)
{
	// Calculate Ambient Light
	return vec4(materialAmbient * light.color, 1);
}


void main(void) 
{
	// calculate texture coordinate
	texCoord0 = aTexCoord;

	// calculate position
	position = matrixModelView * vec4(aVertex, 1.0);
	gl_Position = matrixProjection * position;

	// calculate normal
	normal = normalize(mat3(matrixModelView) * aNormal);

	// calculate depth of water
	waterDepth = waterLevel - aVertex.y;
	snowAltitude = snowLevel - aVertex.y;

	// calculate the observer's altitude above the observed vertex
	float eyeAlt = dot(-position.xyz, mat3(matrixModelView) * vec3(0, 1, 0));

	fogFactor = exp2(-fogDensity * (length(position) * max(waterDepth,0) / max(eyeAlt,waterLevel)));

	gl_ClipDistance[0] = dot(inverse(matrixView) * position, planeClip);

	// calculate light
	color = vec4(0, 0, 0, 1);
	color += AmbientLight(lightAmbient);

}
