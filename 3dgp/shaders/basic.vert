// VERTEX SHADER

#version 330

// Uniforms: Transformation Matrices
uniform mat4 matrixProjection;
uniform mat4 matrixView;
uniform mat4 matrixModelView;

// Uniforms: Material Colours
uniform vec3 materialAmbient;
uniform vec3 materialDiffuse;

uniform vec4 planeClip;

//fog 
uniform float fogDensity;

//Bone Transforms 
#define MAX_BONES 100
uniform mat4 bones[MAX_BONES];


in vec3 aVertex;
in vec3 aNormal;
in vec3 aTangent;
in vec3 aBiTangent;
in vec2 aTexCoord;
in ivec4 aBoneId;
in vec4 aBoneWeight;

out vec2 texCoord0;
out mat3 matrixTangent;
out vec4 color;
out vec4 position;
out vec3 normal;
out float fogFactor;

struct AMBIENT
{
	vec3 color;
};
uniform AMBIENT lightAmbient;

vec4 AmbientLight(AMBIENT light)
{
	return vec4(materialAmbient * light.color,1);
}


void main(void) 
{
    mat4 matrixBone;
	if (aBoneWeight[0] == 0.0)
	{
		matrixBone = mat4(1);
	}
	else
	{
		matrixBone = (bones[aBoneId[0]] * aBoneWeight[0] +
			bones[aBoneId[1]] * aBoneWeight[1] +
			bones[aBoneId[2]] * aBoneWeight[2] +
			bones[aBoneId[3]] * aBoneWeight[3]);
	}
	

	// calculate position
	position = matrixModelView * matrixBone * vec4(aVertex, 1.0);
	gl_Position = matrixProjection * position;

	// calculate normal
	normal = normalize(mat3(matrixModelView) * mat3(matrixBone) * aNormal);

	//calculate fog
	fogFactor = exp2(-fogDensity * length(position));

	// calculate light - start with pitch black
	color = vec4(0, 0, 0, 1);
	color += AmbientLight(lightAmbient);

	gl_ClipDistance[0] = dot(inverse(matrixView) * position, planeClip);


	//calculate texture coordinates
	texCoord0 = aTexCoord;

	// calculate tangent local system transformation
	vec3 tangent = normalize(mat3(matrixModelView) * aTangent);
	vec3 biTangent = normalize(mat3(matrixModelView) * aBiTangent);
	matrixTangent = mat3(tangent, biTangent, normal);

	

}
