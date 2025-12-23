#version 330 core

out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D screenTexture;
uniform vec2 resolution;
uniform float blurRadius;

void main() {
    vec2 tex_offset = 1.0 / resolution; // Size of one texel
    vec4 color = vec4(0.0);
    float weight_sum = 0.0;

    float weights[5] = float[](0.227, 0.194, 0.121, 0.054, 0.016);
    
    for (int i = -2; i <= 2; i++) {
        for (int j = -2; j <= 2; j++) {
            vec2 offset = vec2(i, j) * tex_offset * blurRadius;
            color += texture(screenTexture, TexCoord + offset) * weights[abs(i)] * weights[abs(j)];
            weight_sum += weights[abs(i)] * weights[abs(j)];
        }
    }

    FragColor = color / weight_sum;
}
#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 u_resolution;
uniform vec2 u_mouse;
uniform float u_time;

void main() {
    vec2 st = gl_FragCoord.xy/u_resolution.xy;
    st.x *= u_resolution.x/u_resolution.y;

    vec3 color = vec3(0.);
    color = vec3(st.x,st.y,abs(sin(u_time)));

    gl_FragColor = vec4(color,1.0);
}