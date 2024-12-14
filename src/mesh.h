#ifndef MESH_H
#define MESH_H

#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <utility>
#include <glad/glad.h>

// GLM Stuff
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Mesh
{
public:
    Mesh();

    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<float> texCoords;
    std::vector<std::vector<unsigned int>> faces; // Changed from indices to faces
    std::set<std::pair<unsigned int, unsigned int>> edges; // New edges property

    unsigned int VAO;
    unsigned int VBO;
    unsigned int EBO;

    bool uploaded = false;
    bool has_normals = false;

    void add_vertex(float x, float y, float z);
    void add_face(unsigned int i0, unsigned int i1, unsigned int i2);
    void add_faceq(unsigned int i0, unsigned int i1, unsigned int i2, unsigned int i3);
    void gen_normals();

    // GPU Stuff
    void upload();

    // IO Stuff
    void to_obj(std::string filepath);
    void to_objq(std::string filepath);

    static Mesh makeCube();

private:
    void add_edge(unsigned int i0, unsigned int i1);
};

Mesh::Mesh() {
    vertices = std::vector<float>();
    normals = std::vector<float>();
    texCoords = std::vector<float>();
    faces = std::vector<std::vector<unsigned int>>(); // Initialize faces
    edges = std::set<std::pair<unsigned int, unsigned int>>(); // Initialize edges
}

void Mesh::add_edge(unsigned int i0, unsigned int i1) {
    if (i0 > i1) std::swap(i0, i1); // Ensure the smaller index comes first
    edges.insert({i0, i1});
}

void Mesh::add_face(unsigned int i0, unsigned int i1, unsigned int i2) {
    faces.push_back({i0, i1, i2});
    add_edge(i0, i1);
    add_edge(i1, i2);
    add_edge(i2, i0);
}

void Mesh::add_faceq(unsigned int i0, unsigned int i1, unsigned int i2, unsigned int i3) {
    faces.push_back({i0, i1, i2, i3});
    add_edge(i0, i1);
    add_edge(i1, i2);
    add_edge(i2, i3);
    add_edge(i3, i0);
}

void Mesh::gen_normals() {
    // Initialize normals to zero
    normals.resize(vertices.size(), 0.0f);

    // Calculate face normals and add them to the vertex normals
    for (const auto& face : faces) {
        if (face.size() == 3) {
            unsigned int i0 = face[0];
            unsigned int i1 = face[1];
            unsigned int i2 = face[2];

            glm::vec3 v0(vertices[3 * i0], vertices[3 * i0 + 1], vertices[3 * i0 + 2]);
            glm::vec3 v1(vertices[3 * i1], vertices[3 * i1 + 1], vertices[3 * i1 + 2]);
            glm::vec3 v2(vertices[3 * i2], vertices[3 * i2 + 1], vertices[3 * i2 + 2]);

            glm::vec3 edge1 = v1 - v0;
            glm::vec3 edge2 = v2 - v0;
            glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

            normals[3 * i0] += normal.x;
            normals[3 * i0 + 1] += normal.y;
            normals[3 * i0 + 2] += normal.z;

            normals[3 * i1] += normal.x;
            normals[3 * i1 + 1] += normal.y;
            normals[3 * i1 + 2] += normal.z;

            normals[3 * i2] += normal.x;
            normals[3 * i2 + 1] += normal.y;
            normals[3 * i2 + 2] += normal.z;
        }
    }

    // Normalize the vertex normals
    for (size_t i = 0; i < normals.size(); i += 3) {
        glm::vec3 normal(normals[i], normals[i + 1], normals[i + 2]);
        normal = glm::normalize(normal);

        normals[i] = normal.x;
        normals[i + 1] = normal.y;
        normals[i + 2] = normal.z;
    }

    has_normals = true;
}

void Mesh::upload() {
    // Upload the mesh to the GPU generating the VAO, VBO and EBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    // Bind the VAO
    glBindVertexArray(VAO);

    // Bind the VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // Bind the EBO
    std::vector<unsigned int> indices;
    for (const auto& face : faces) {
        indices.insert(indices.end(), face.begin(), face.end());
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Set the vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // Unbind the VAO
    glBindVertexArray(0);
}

void Mesh::add_vertex(float x, float y, float z) {
    vertices.push_back(x);
    vertices.push_back(y);
    vertices.push_back(z);
}

Mesh Mesh::makeCube()
{
    Mesh mesh = Mesh();

    std::cout << "Making Cube" << std::endl;

    // Front face
    mesh.add_vertex(-0.5f, -0.5f, 0.5f);
    mesh.add_vertex(0.5f, -0.5f, 0.5f);
    mesh.add_vertex(0.5f, 0.5f, 0.5f);
    mesh.add_vertex(-0.5f, 0.5f, 0.5f);
    mesh.add_faceq(0, 1, 2, 3);

    // Back face
    mesh.add_vertex(-0.5f, -0.5f, -0.5f);
    mesh.add_vertex(0.5f, -0.5f, -0.5f);
    mesh.add_vertex(0.5f, 0.5f, -0.5f);
    mesh.add_vertex(-0.5f, 0.5f, -0.5f);
    mesh.add_faceq(4, 5, 6, 7);

    // Right face
    mesh.add_vertex(0.5f, -0.5f, 0.5f);
    mesh.add_vertex(0.5f, -0.5f, -0.5f);
    mesh.add_vertex(0.5f, 0.5f, -0.5f);
    mesh.add_vertex(0.5f, 0.5f, 0.5f);
    mesh.add_faceq(8, 9, 10, 11);

    // Left face
    mesh.add_vertex(-0.5f, -0.5f, 0.5f);
    mesh.add_vertex(-0.5f, -0.5f, -0.5f);
    mesh.add_vertex(-0.5f, 0.5f, -0.5f);
    mesh.add_vertex(-0.5f, 0.5f, 0.5f);
    mesh.add_faceq(12, 13, 14, 15);

    // Top face
    mesh.add_vertex(-0.5f, 0.5f, 0.5f);
    mesh.add_vertex(0.5f, 0.5f, 0.5f);
    mesh.add_vertex(0.5f, 0.5f, -0.5f);
    mesh.add_vertex(-0.5f, 0.5f, -0.5f);
    mesh.add_faceq(16, 17, 18, 19);

    // Bottom face
    mesh.add_vertex(-0.5f, -0.5f, 0.5f);
    mesh.add_vertex(0.5f, -0.5f, 0.5f);
    mesh.add_vertex(0.5f, -0.5f, -0.5f);
    mesh.add_vertex(-0.5f, -0.5f, -0.5f);
    mesh.add_faceq(20, 21, 22, 23);

    return mesh;
}

void Mesh::to_obj(std::string filepath) {
    // Save the mesh to an obj file

    std::ofstream file;
    file.open(filepath);

    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return;
    }

    for (int i = 0; i < vertices.size(); i += 3) {
        file << "v " << vertices[i] << " " << vertices[i + 1] << " " << vertices[i + 2] << std::endl;
    }

    for (int i = 0; i < normals.size(); i += 3) {
        file << "vn " << normals[i] << " " << normals[i + 1] << " " << normals[i + 2] << std::endl;
    }

    for (int i = 0; i < texCoords.size(); i += 2) {
        file << "vt " << texCoords[i] << " " << texCoords[i + 1] << std::endl;
    }

    for (const auto& face : faces) {
        file << "f";
        for (const auto& index : face) {
            file << " " << index + 1;
        }
        file << std::endl;
    }

    file.close();
}

void Mesh::to_objq(std::string filepath) {
    // Save the mesh to an obj file with quads
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return;
    }

    // Write vertices
    for (size_t i = 0; i < vertices.size(); i += 3) {
        file << "v " << vertices[i] << " " << vertices[i + 1] << " " << vertices[i + 2] << std::endl;
    }

    // Write normals
    if (has_normals) {
        for (size_t i = 0; i < normals.size(); i += 3) {
            file << "vn " << normals[i] << " " << normals[i + 1] << " " << normals[i + 2] << std::endl;
        }
    }

    // Write faces as quads
    for (const auto& face : faces) {
        if (face.size() == 4) {
            file << "f " 
                 << face[0] + 1 << "//" << face[0] + 1 << " "
                 << face[1] + 1 << "//" << face[1] + 1 << " "
                 << face[2] + 1 << "//" << face[2] + 1 << " "
                 << face[3] + 1 << "//" << face[3] + 1 << std::endl;
        }
    }

    file.close();
}

#endif