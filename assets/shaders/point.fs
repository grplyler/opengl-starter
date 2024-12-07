#version 330 core

in vec3 frag_position;  // Interpolated world-space position from the vertex shader
out vec4 frag_color;

uniform vec3 sphere_center;     // Center of the sphere in world space
uniform vec3 light_direction;   // Light direction (normalized)
uniform vec3 light_color;       // Light color
uniform vec3 sphere_color;      // Base color of the sphere
uniform float sphere_radius;    // Radius of the sphere
uniform vec3 view_position;     // Camera position (world space)
uniform float specular_strength; // Specular strength

void main() {
    // Screen-space relative position of the fragment within the sphere's point
    vec2 screen_pos = gl_PointCoord * 2.0 - vec2(1.0); // Map from [0,1] to [-1,1]
    float dist = dot(screen_pos, screen_pos);

    // Discard fragments outside the sphere's circular boundary
    if (dist > 1.0) {
        discard;
    }

    // Compute the z-coordinate of the sphere's surface (z^2 = 1 - x^2 - y^2)
    float z = sqrt(1.0 - dist);
    vec3 normal = normalize(vec3(screen_pos, z)); // Normal vector for the sphere's surface

    // Lighting calculations
    // Ambient lighting
    vec3 ambient = 0.1 * sphere_color;

    // Diffuse lighting
    float diffuse = max(dot(normal, normalize(-light_direction)), 0.0); // Light contribution
    vec3 diffuse_light = diffuse * light_color * sphere_color;

    // Make the diffuse based on the normal
    // float diffuse = max(dot(normal, normalize(-light_direction)), 0.1); // Light contribution
    // vec3 diffuse_light = diffuse * normal;

    // Specular lighting
    vec3 view_dir = normalize(view_position - frag_position); // Direction to the camera
    vec3 reflect_dir = reflect(normalize(light_direction), normal); // Reflection of the light
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32.0); // Shininess factor
    vec3 specular = specular_strength * spec * light_color;

    // Combine lighting components
    vec3 lighting = ambient + diffuse_light + specular;

    // Final color
    frag_color = vec4(lighting, 1.0);
}
