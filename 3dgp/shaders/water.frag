#version 330

//Uniforms: water
uniform vec4 waterColor;
uniform sampler2D reflectionTexture; // Reflection texture
uniform vec2 screenRes = vec2(1280, 720);


// Input Variables (received from Vertex Shader)
in vec4 color;
in vec4 position;
in vec3 normal;
in vec2 texCoord0;
in float reflFactor;			// reflection coefficient

// Output Variable (sent down through the Pipeline)
out vec4 outColor;

void main(void) 
{
	outColor = color;

	// Sample the reflection texture
	vec4 reflectionColor = texture(reflectionTexture, gl_FragCoord.xy / screenRes);

	//reflectionColor.a *= 0.8;
	outColor = mix(waterColor, reflectionColor, reflFactor);


}
