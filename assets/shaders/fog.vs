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
uniform vec3 windVec;
uniform float windTime;

void main()
{
    vec3 pos = vertexPosition;

    float sway = vertexColor.a;
    float ws = min(length(windVec), 16.0);
    vec3 wd = ws > 0.1 ? normalize(windVec) : vec3(1.0, 0.0, 0.0);

    if (sway > 0.01) {
        float h = 1.0 - fract(vertexTexCoord.y * 16.0);
        float n = pos.x * 1.7 + pos.z * 2.3;
        float osc = sin(windTime * (3.5 + ws * 0.35) + n) * 0.5 + sin(windTime * (7.0 + ws * 0.5) + n * 1.4) * 0.35;
        float bend = sway * ws * 0.014 * h;
        float turb = sway * ws * 0.008 * osc * h;
        vec3 d = wd * (bend + turb);
        d.y += abs(osc) * sway * 0.005 * ws * h;
        pos += d;
    }

    fragColor = vec4(vertexColor.rgb, 1.0);
    fragPosition = vec3(matModel * vec4(pos, 1.0));
    fragTexCoord = vertexTexCoord;
    fragNormal = vertexNormal;
    gl_Position = mvp * vec4(pos, 1.0);
}
