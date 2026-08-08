#version 330

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform vec3 viewPos;
uniform vec4 fogColor;
uniform float fogStart;
uniform float fogEnd;

uniform float waterTime;
uniform float waterFrames;
uniform float windStrength;
uniform vec3 sunDir;
uniform vec4 skyTint;
uniform int underwater;

out vec4 finalColor;

vec4 waterSample(vec2 uv, float frame)
{
    vec2 cell = floor(fract(uv) * 16.0);
    float u = (cell.x + 0.5) / 16.0;
    float v = (frame * 16.0 + cell.y + 0.5) / (16.0 * waterFrames);
    return texture(texture0, vec2(u, v));
}

void main()
{
    vec3 nrm = normalize(fragNormal);
    vec3 viewDir = normalize(viewPos - fragPosition);

    vec2 planar = fragPosition.xz;
    if (abs(nrm.y) < 0.5) planar = (abs(nrm.x) > 0.5) ? fragPosition.zy : fragPosition.xy;

    float wind = clamp(windStrength / 12.0, 0.15, 1.3);
    float f = mod(waterTime * (6.0 + wind * 9.0), waterFrames);
    float frame = floor(f);
    float frameNext = mod(frame + 1.0, waterFrames);
    float blend = fract(f);

    vec2 uvA = planar * 0.24 + vec2(waterTime * 0.011, waterTime * 0.007) * wind;
    vec2 uvB = planar * 0.085 - vec2(waterTime * 0.006, -waterTime * 0.0045) * wind;

    vec4 tA = mix(waterSample(uvA, frame), waterSample(uvA, frameNext), blend);
    vec4 tB = mix(waterSample(uvB, frame), waterSample(uvB, frameNext), blend);
    vec4 texelColor = mix(tA, tB, 0.42);

    vec3 baseColor = texelColor.rgb * colDiffuse.rgb * max(fragColor.rgb, vec3(0.42));

    float facing = clamp(dot(nrm, viewDir), 0.0, 1.0);
    float fresnel = pow(1.0 - facing, 4.0);

    vec3 lit = mix(baseColor, skyTint.rgb, fresnel * 0.45);

    float luma = dot(texelColor.rgb, vec3(0.333));
    vec3 halfway = normalize(sunDir + viewDir);
    float spec = pow(max(dot(nrm, halfway), 0.0), 64.0);
    lit += vec3(1.0, 0.97, 0.90) * spec * smoothstep(0.46, 0.62, luma) * 0.32 * fragColor.r;

    float alpha = texelColor.a * colDiffuse.a;
    alpha = clamp(alpha + fresnel * 0.28, 0.0, 1.0);

    if (underwater == 1) {
        lit = mix(lit, fogColor.rgb, 0.45);
        alpha = min(alpha, 0.55);
    }

    float dist = length(viewPos - fragPosition);
    float dRel = clamp((dist - fogStart) / (fogEnd - fogStart), 0.0, 1.0);
    float fogFactor = 1.0 - exp(-dRel * dRel * 3.2);
    lit = mix(lit, fogColor.rgb, fogFactor);

    finalColor = vec4(lit, alpha);
}
