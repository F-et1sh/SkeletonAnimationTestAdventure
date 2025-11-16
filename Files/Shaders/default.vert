#version 460

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aColor;

layout(location = 3) in uvec4 aJoints;
layout(location = 4) in vec4  aWeights;

out vec3 crntPos;
out vec3 Normal;
out vec3 color;

uniform mat4 camMatrix;
uniform mat4 model;

uniform mat4 bones[128];

void main() {

    mat4 skinMat =
        bones[aJoints.x] * aWeights.x +
        bones[aJoints.y] * aWeights.y +
        bones[aJoints.z] * aWeights.z +
        bones[aJoints.w] * aWeights.w;

    vec4 skinnedPos = skinMat * vec4(aPos, 1.0);
    vec4 skinnedNormal = skinMat * vec4(aNormal, 0.0);

    // output
    Normal = skinnedNormal.xyz;
    color = aColor;

    gl_Position = camMatrix * model * skinnedPos;
}
