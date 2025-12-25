#version 330 core

uniform vec2 u_resolution;
uniform vec2 u_mouse;
uniform float u_time;

void main(){
    vec2 st = gl_FragCoord.xy/u_resolution;
    vec2 mouse_norm = u_mouse/u_resolution;
    gl_FragColor = vec4(st.x,st.y,abs(sin(mouse_norm.x*u_time)),1.0);
} 