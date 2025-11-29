#version 460

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texCoord;
layout(location = 3) in uvec4 a_joints;
layout(location = 4) in vec4 a_weights;
layout(location = 5) in vec4 a_tangent;

out vec3 i_position;
out vec2 i_texCoord;
out mat3 i_TBN;

#define JOINTS_COUNT 128

uniform mat4 u_cameraMatrix;
uniform mat4 u_model;
uniform mat4 u_bones[JOINTS_COUNT];

void main() {
    mat4 skin_matrix =
        u_bones[a_joints.x] * a_weights.x +
        u_bones[a_joints.y] * a_weights.y +
        u_bones[a_joints.z] * a_weights.z +
        u_bones[a_joints.w] * a_weights.w;

    vec4 skinned_position = skin_matrix * vec4(a_position, 1.0f);

    mat3 skin = mat3(skin_matrix);
    vec3 skinned_normal  = normalize(skin * a_normal);
    vec3 skinned_tangent = normalize(skin * a_tangent.xyz);

    vec3 T = normalize(mat3(u_model) * skinned_tangent.xyz);
    vec3 N = normalize(mat3(u_model) * skinned_normal);
    vec3 B = cross(N, T) * a_tangent.w;

    i_TBN = mat3(T, B, N);

    i_texCoord = a_texCoord;

    vec4 world_pos = u_model * skinned_position;
    i_position = world_pos.xyz;

    gl_Position = u_cameraMatrix * world_pos;
}