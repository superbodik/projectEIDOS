#version 330

// Inputs from vertex shader
in vec2 fragTexCoord;
in vec4 fragColor;

// Output
out vec4 finalColor;

// Uniforms
uniform sampler2D texture0;

void main()
{
    vec4 texColor = texture(texture0, fragTexCoord);
    finalColor = texColor * fragColor;
}
