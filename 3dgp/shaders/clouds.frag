#version 330

in float age;
flat in int textureIndex;
uniform sampler2D textures[4];
in float normalizedDistance;    // Normalized distance for opacity calculation
in float normalizedDistanceFromCentre;    // Normalized distance for opacity calculation

out vec4 outColor;

void main()
{
    // Sample the texture based on the texture index
    vec4 texColor = texture(textures[textureIndex], gl_PointCoord);

    // Calculate opacity based on distance from the camera (closer = more opaque)
    float cameraOpacity = 1.0 - normalizedDistance;

    // Calculate opacity based on distance from the cluster center (closer = more opaque)
    float centerOpacity = 1.0 - normalizedDistanceFromCentre; // centre isnt moving need to adjust it 

    // Combine both opacities (multiplicative effect)
    float finalOpacity = cameraOpacity * centerOpacity;

    // Apply the combined opacity to the cloud
    outColor = vec4(texColor.rgb, texColor.a * finalOpacity);
    
}
