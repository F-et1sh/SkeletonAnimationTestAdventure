#version 460

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in uvec4 a_Joints;
layout(location = 4) in vec4 a_Weights;

out vec3 i_Position;
out vec3 i_Normal;
out vec2 i_TexCoord;

uniform mat4 u_CameraMatrix;
uniform mat4 u_Model;
uniform mat4 u_Bones[128];

void main() {
    mat4 skin_matrix =
        u_Bones[a_Joints.x] * a_Weights.x +
        u_Bones[a_Joints.y] * a_Weights.y +
        u_Bones[a_Joints.z] * a_Weights.z +
        u_Bones[a_Joints.w] * a_Weights.w;

    vec4 skinned_position = skin_matrix * vec4(a_Position, 1.0f);

    mat3 skin = mat3(skin_matrix);
    vec3 skinned_normal = normalize(skin * a_Normal);

    i_Normal = skinned_normal;
    i_TexCoord = a_TexCoord;

    vec4 world_pos = u_Model * skinned_position;
    gl_Position = u_CameraMatrix * world_pos;

    i_Position = world_pos.xyz;
}