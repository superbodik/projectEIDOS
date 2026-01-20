#version 330

in vec4 fragColor;
in vec3 fragPosition;

out vec4 finalColor;

uniform vec3 viewPos;
uniform vec4 fogColor;
uniform float fogStart;
uniform float fogEnd;

void main() {
    float dist = distance(viewPos, fragPosition);
    float fogFactor = clamp((fogEnd - dist) / (fogEnd - fogStart), 0.0, 1.0);
    finalColor = vec4(mix(fogColor.rgb, fragColor.rgb, fogFactor), fragColor.a);
}