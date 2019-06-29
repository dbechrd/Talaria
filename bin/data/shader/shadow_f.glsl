#version 330 core

// TODO: Why do I need this when using gl_FragDepth?
layout(location = 0) out float final_depth;

void main() {
    //final_depth = 0.2; //gl_FragCoord.z;
    gl_FragDepth = gl_FragCoord.z / 20;
}