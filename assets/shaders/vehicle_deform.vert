#version 300 es
precision highp float;

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in vec4 a_DeformIndices;

uniform mat4 u_WorldViewProj;
uniform mat4 u_World;
uniform sampler2D u_DeformTexture;
uniform vec4 u_DeformProperties;
uniform float u_Bias;

out vec3 v_WorldPos;
out vec3 v_Normal;
out vec2 v_TexCoord;

void main() {
    // Dynamic vehicle deformation from damage map (CTAB deform.ovs translation)
    vec2 deformUV = a_DeformIndices.xy * u_DeformProperties.xy + u_DeformProperties.zw;
    vec4 displacement = texture(u_DeformTexture, deformUV);

    vec3 deformedPos = a_Position + displacement.xyz * u_Bias;

    v_WorldPos = (u_World * vec4(deformedPos, 1.0)).xyz;
    v_Normal = normalize((u_World * vec4(a_Normal, 0.0)).xyz);
    v_TexCoord = a_TexCoord;

    gl_Position = u_WorldViewProj * vec4(deformedPos, 1.0);
}
