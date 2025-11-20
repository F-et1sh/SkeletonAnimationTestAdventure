#version 460

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in uvec4 a_Joints;
layout(location = 4) in vec4 a_Weights;
layout(location = 5) in vec4 a_Tangent;

out vec3 i_Position;
out vec2 i_TexCoord;
out mat3 i_TBN;

uniform mat4 u_CameraMatrix;
uniform mat4 u_Model;
uniform mat4 u_Bones[128];

void main() {
    mat4 skin_matrix =
        u_Bones[a_Joints.x] * a_Weights.x +
        u_Bones[a_Joints.y] * a_Weights.y +
        u_Bones[a_Joints.z] * a_Weights.z +
        u_Bones[a_Joints.w] * a_Weights.w;

    vec4 skinned_position = skin_matrix * vec4(a_Position, 1.0);

    mat3 skin = mat3(skin_matrix);
    vec3 skinned_normal  = normalize(skin * a_Normal);
    vec3 skinned_tangent = normalize(skin * a_Tangent.xyz);

    vec3 T = normalize(mat3(u_Model) * skinned_tangent);
    vec3 N = normalize(mat3(u_Model) * skinned_normal);
    vec3 B = cross(N, T) * a_Tangent.w;

    i_TBN = mat3(T, B, N);

    i_TexCoord = a_TexCoord;

    vec4 world_pos = u_Model * skinned_position;
    i_Position = world_pos.xyz;

    gl_Position = u_CameraMatrix * world_pos;
}