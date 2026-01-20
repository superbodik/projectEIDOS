#version 330

in vec3 vertexPosition;
in vec4 vertexColor;

out vec4 fragColor;
out vec3 fragPosition;

uniform mat4 mvp;
uniform mat4 matModel;

void main() {
    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));
    fragColor = vertexColor;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}