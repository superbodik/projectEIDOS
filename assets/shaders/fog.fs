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
uniform vec3 sunDir;

uniform sampler2D shadowMap;
uniform mat4 shadowMVP;
uniform float shadowStrength;

uniform float exposure;
uniform float saturation;

out vec4 finalColor;

float SampleShadow(vec3 world, float ndl)
{
    if (shadowStrength < 0.01) return 1.0;

    vec4 lp = shadowMVP * vec4(world, 1.0);
    vec3 proj = lp.xyz / lp.w;
    proj = proj * 0.5 + 0.5;

    if (proj.z > 1.0 || proj.x < 0.0 || proj.x > 1.0 || proj.y < 0.0 || proj.y > 1.0)
        return 1.0;

    float bias = max(0.0016 * (1.0 - ndl), 0.0004);
    vec2 texel = 1.0 / vec2(textureSize(shadowMap, 0));

    float lit = 0.0;
    for (int y = -1; y <= 1; y++)
    {
        for (int x = -1; x <= 1; x++)
        {
            float d = texture(shadowMap, proj.xy + vec2(x, y) * texel).r;
            lit += (proj.z - bias > d) ? 0.0 : 1.0;
        }
    }
    lit /= 9.0;

    return mix(1.0, lit, shadowStrength);
}

vec3 Tonemap(vec3 c)
{
    c *= exposure;
    float a = 2.51;
    float b = 0.03;
    float y = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((c * (a * c + b)) / (c * (y * c + d) + e), 0.0, 1.0);
}

void main()
{
    vec4 texelColor = texture(texture0, fragTexCoord);
    if (texelColor.a < 0.1) discard;

    vec3 nrm = normalize(fragNormal);
    float sunUp = clamp(sunDir.y * 1.5, 0.0, 1.0);
    float ndl = clamp(dot(nrm, sunDir), 0.0, 1.0);

    float horizon = 1.0 - clamp(abs(sunDir.y) * 2.2, 0.0, 1.0);
    vec3 sunTint = mix(vec3(1.05, 0.98, 0.85), vec3(1.25, 0.66, 0.36), horizon);

    vec3 skyCol = mix(vec3(0.26, 0.30, 0.42), vec3(0.50, 0.60, 0.78), sunUp);
    vec3 gndCol = mix(vec3(0.16, 0.16, 0.19), vec3(0.32, 0.29, 0.24), sunUp);
    float up = nrm.y * 0.5 + 0.5;
    vec3 ambient = mix(gndCol, skyCol, up);

    float shade = SampleShadow(fragPosition, ndl);
    vec3 sun = sunTint * ndl * shade * 0.85 * sunUp;

    float bounce = clamp(-dot(nrm, sunDir), 0.0, 1.0) * 0.12 * sunUp;
    vec3 light = ambient + sun + sunTint * bounce;

    vec3 baseColor = texelColor.rgb * colDiffuse.rgb * fragColor.rgb * light;

    baseColor = Tonemap(baseColor);

    float luma = dot(baseColor, vec3(0.2126, 0.7152, 0.0722));
    baseColor = mix(vec3(luma), baseColor, saturation);

    float dRel = clamp((length(viewPos - fragPosition) - fogStart) / (fogEnd - fogStart), 0.0, 1.0);
    float fogFactor = 1.0 - exp(-dRel * dRel * 3.2);

    finalColor = vec4(mix(baseColor, fogColor.rgb, fogFactor), texelColor.a);
}
