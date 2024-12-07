#version 330 core
layout(location = 0) in vec3 position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float sphere_radius;

out vec3 frag_position; // Pass the world-space position to the fragment shader

void main() {
    gl_Position = projection * view * model * vec4(position, 1.0); // Transform point to clip space
    frag_position = vec3(model * vec4(position, 1.0));            // World-space position

    // Scale the point
    gl_PointSize = sphere_radius / length(frag_position);
}
