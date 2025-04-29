#version 330

// Uniforms: Transformation Matrices
uniform mat4 matrixProjection;
uniform mat4 matrixView;
uniform mat4 matrixModelView;

// Particle-specific Uniforms
uniform float time;	// Animation Time
//uniform float particleLifetime;	 // max particle lifetime
uniform vec3 cameraPosition;      // Camera position in world space
uniform float clusterRadius;
uniform float maxDistance;        // Maximum distance for opacity drop-off
uniform float baseParticleSize; // Base size of the particle
uniform float sizeFalloffFactor; // Factor to control size falloff with distance
uniform vec4 planeClip;      // clipping plane for planar refelction


// Special Vertex Attributes
//in float aStartTime;					// Particle "birth" time
in vec3 aVelocity;					// Particle initial velocity
in vec3 aPosition;					// Particle position
in int aTextureIndex;               // Texture index for the particle
in vec3 centreCloudPos;


// Output Variable (sent to Fragment Shader)
out float age;					// age of the particle (0..1)
flat out int textureIndex; 
out float normalizedDistance;   // Normalized distance for opacity calculation
out float normalizedDistanceFromCentre;   // Normalized distance for opacity calculation


void main()
{
    vec3 position = aPosition + aVelocity * time;

    textureIndex = aTextureIndex;

    // Calculate distance from camera
    float distanceFromCamera = length(position - cameraPosition);

    float distanceFromCentre = length(position - centreCloudPos);

    // Normalize distance (clamp to [0, 1])
    normalizedDistance = clamp(distanceFromCamera / maxDistance, 0.0, 1.0);

    normalizedDistanceFromCentre = clamp(distanceFromCentre / clusterRadius, 0.0, 1.0);

    // Calculate the particle size based on distance
    gl_PointSize = baseParticleSize / (1.0 + sizeFalloffFactor * distanceFromCamera);

    // Set the final position
    gl_Position = matrixProjection * matrixView * vec4(position, 1.0);

    gl_ClipDistance[0] = dot(inverse(matrixView) * vec4(position, 1.0), planeClip);

}
