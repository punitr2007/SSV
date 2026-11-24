#version 300 es
precision highp float;

in vec3 v_WorldPos;
in vec3 v_Normal;
in vec2 v_TexCoord;

uniform sampler2D u_DiffuseMap;
uniform vec3 u_LightDir;
uniform vec4 u_CarPaintColor;

out vec4 fragColor;

void main() {
    vec4 albedo = texture(u_DiffuseMap, v_TexCoord);
    vec3 N = normalize(v_Normal);
    vec3 L = normalize(-u_LightDir);

    float diff = max(dot(N, L), 0.2);
    vec3 color = albedo.rgb * u_CarPaintColor.rgb * diff;

    fragColor = vec4(color, albedo.a);
}
