#version 460

out vec4 fragColor;

in vec3 i_position;
in vec2 i_texCoord;
in mat3 i_TBN;

uniform sampler2D u_baseColorTex;
uniform sampler2D u_metallicRoughnessTex;
uniform sampler2D u_normalTex;
uniform sampler2D u_occlusionTex;
uniform sampler2D u_emissiveTex;

uniform vec4 u_baseColorFactor;
uniform float u_metallicFactor;
uniform float u_roughnessFactor;

uniform vec3 u_lightDirection;
uniform vec3 u_lightColor;
uniform vec3 u_cameraPosition;

#define PI 3.14159265358979323846

vec3 getNormal() {
    vec3 n = texture(u_normalTex, i_texCoord).xyz * 2.0f - 1.0f;
    return normalize(i_TBN * n);
}

vec3 fresnelSchlick(float cos_theta, vec3 F0) {
    return F0 + (1.0f - F0) * pow(1.0f - cos_theta, 5.0f);
}

float DistributionGGX(vec3 N, vec3 H, float rough) {
    float a      = rough * rough;
    float a2     = a * a;
    float N_dot_H  = max(dot(N, H), 0.0f);
    float denom  = (N_dot_H * N_dot_H * (a2 - 1.0f) + 1.0f);
    return a2 / (PI * denom * denom);
}

float GeometrySchlickGGX(float N_dot_V, float rough) {
    float r = (rough + 1.0f);
    float k = (r * r) / 8.0f;
    return N_dot_V / (N_dot_V * (1.0f - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float rough) {
    float N_dot_V = max(dot(N, V), 0.0f);
    float N_dot_L = max(dot(N, L), 0.0f);
    return GeometrySchlickGGX(N_dot_V, rough) * GeometrySchlickGGX(N_dot_L, rough);
}

void main() {
    vec3 N = getNormal();
    vec3 V = normalize(u_cameraPosition - i_position);
    vec3 L = normalize(-u_lightDirection);
    vec3 H = normalize(V + L);

    float N_dot_L = max(dot(N, L), 0.0f);
    if (N_dot_L <= 0.0f) {
        fragColor = vec4(0.0f);
        return;
    }

    vec3 base_color = pow(texture(u_baseColorTex, i_texCoord).rgb, vec3(2.2f));
    base_color *= u_baseColorFactor.rgb;
    float alpha = texture(u_baseColorTex, i_texCoord).a * u_baseColorFactor.a;

    vec4 mr = texture(u_metallicRoughnessTex, i_texCoord);
    
    float roughness = mr.g * u_roughnessFactor;
    float metallic  = mr.b * u_metallicFactor;

    roughness = clamp(roughness, 0.0f, 1.0f);
    metallic  = clamp(metallic,  0.0f, 1.0f);

	vec3 F0 = mix(vec3(0.04f), base_color, metallic);

    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3  F   = fresnelSchlick(max(dot(H, V), 0.0f), F0);
    
    vec3 specular = (NDF * G * F) / max(4.0f * max(dot(N,V), 0.0f) * N_dot_L, 0.001f);

    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);

    vec3 irradiance = u_lightColor * N_dot_L;
    vec3 diffuse  = base_color / PI;
    vec3 lighting = (kD * diffuse + specular) * irradiance;

    vec3 color = lighting;

    color += texture(u_emissiveTex, i_texCoord).rgb;
    color = pow(color, vec3(1.0f / 2.2f));
    
    fragColor = vec4(color, 1.0f);
}