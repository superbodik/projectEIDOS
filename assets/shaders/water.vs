#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

out vec3 fragPosition;
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragNormal;

uniform mat4 mvp;
uniform mat4 matModel;
uniform float waterTime;
uniform float windStrength;

void main()
{
    vec3 pos = vertexPosition;
    vec3 world = vec3(matModel * vec4(pos, 1.0));

    if (vertexNormal.y > 0.5) {
        float wind = clamp(windStrength / 12.0, 0.1, 1.4);
        float w1 = sin(world.x * 0.6 + waterTime * (1.1 + wind));
        float w2 = sin(world.z * 0.8 - waterTime * (0.8 + wind * 0.7));
        float w3 = sin((world.x + world.z) * 0.4 + waterTime * 1.7);
        pos.y -= 0.05 + (0.012 + 0.05 * wind) * (w1 * 0.5 + w2 * 0.35 + w3 * 0.25);
        world = vec3(matModel * vec4(pos, 1.0));
    }

    fragPosition = world;
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    fragNormal = vertexNormal;

    gl_Position = mvp * vec4(pos, 1.0);
}
