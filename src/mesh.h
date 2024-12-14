#ifndef MESH_H
#define MESH_H

#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <glad/glad.h>

class Mesh
{
public:
    Mesh();
    ~Mesh();

    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<float> texCoords;
    std::vector<unsigned int> indices;

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

    static Mesh makeCube();

};

Mesh::Mesh() {
    vertices = std::vector<float>();
    normals = std::vector<float>();
    texCoords = std::vector<float>();
    indices = std::vector<unsigned int>();
}

Mesh::~Mesh() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
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

void Mesh::add_face(unsigned int i0, unsigned int i1, unsigned int i2) {
    indices.push_back(i0);
    indices.push_back(i1);
    indices.push_back(i2);
}

void Mesh::add_faceq(unsigned int i0, unsigned int i1, unsigned int i2, unsigned int i3) {
    add_face(i0, i1, i2);
    add_face(i0, i2, i3);
}

// IO
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

    for (int i = 0; i < indices.size(); i += 3) {
        file << "f " << indices[i] + 1 << " " << indices[i + 1] + 1 << " " << indices[i + 2] + 1 << std::endl;
    }

    file.close();
}

Mesh Mesh::makeCube()
{
    Mesh mesh;

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
}
#endif