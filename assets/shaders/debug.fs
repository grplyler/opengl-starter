// filepath: /Users/ryan/code/opengl-starter/shaders/normal_viz.frag
#version 330 core
out vec4 FragColor;
uniform vec3 lineColor;

void main() {
    FragColor = vec4(lineColor, 1.0);
}