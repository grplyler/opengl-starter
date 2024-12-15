#ifndef LIGHT_H
#define LIGHT_H

// GLM Stuff
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"

// Basic Point Light class
class Light {
    public:
        Light(glm::vec3 position, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float intensity) : position(position), ambient(ambient), diffuse(diffuse), specular(specular), intensity(intensity) {}
        glm::vec3 position;
        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;
        float intensity;
        Shader shader;

        void set_shader(Shader shader) {
            this->shader = shader;
        }

};

#endif