#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>

#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;

    Vertex(const glm::vec3& pos) : position(pos), normal(0.0f), texCoord(0.0f) {}
};

class Edge {
public:
    int vertex1; // Index of the first vertex
    int vertex2; // Index of the second vertex
    int face1;   // Index of the first face
    int face2;   // Index of the second face

    Edge(int v1, int v2) : vertex1(v1), vertex2(v2), face1(-1), face2(-1) {}
};

class Face {
public:
    std::vector<int> vertices; // Indices of vertices
    std::vector<int> edges;    // Indices of edges

    Face(const std::vector<int>& verts) : vertices(verts) {}
};

class Mesh {
public:
    Mesh();
    ~Mesh(); // Destructor to clean up GPU resources

    std::vector<Vertex> vertices;
    std::vector<Edge> edges;
    std::vector<Face> faces;

    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;

    bool uploaded = false;
    bool has_normals = false;

    void add_vertex(const glm::vec3& position);
    void add_face(const std::vector<int>& vertexIndices);
    void gen_normals();

    // GPU Stuff
    void upload();

    // IO Stuff
    void to_obj(const std::string& filepath);
    void to_objq(const std::string& filepath);

    static Mesh makeCube();

private:
    struct pair_hash {
        template <class T1, class T2>
        std::size_t operator()(const std::pair<T1, T2>& pair) const {
            return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
        }
    };

    std::unordered_map<std::pair<int, int>, int, pair_hash> edgeMap; // Optimized edge lookup
    int find_or_create_edge(int v1, int v2);
    glm::vec3 calculate_face_point(int faceIndex);
    glm::vec3 calculate_edge_point(int edgeIndex);
    glm::vec3 calculate_vertex_point(int vertexIndex);
};

Mesh::Mesh() {
    // Empty constructor
}

Mesh::~Mesh() {
    if (uploaded) {
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glDeleteVertexArrays(1, &VAO);
    }
}

void Mesh::add_vertex(const glm::vec3& position) {
    vertices.emplace_back(position);
}

void Mesh::add_face(const std::vector<int>& vertexIndices) {
    if (vertexIndices.size() < 3) {
        std::cerr << "Error: Face must have at least 3 vertices." << std::endl;
        return;
    }

    faces.emplace_back(vertexIndices);
    int faceIndex = faces.size() - 1;

    for (size_t i = 0; i < vertexIndices.size(); ++i) {
        int v1 = vertexIndices[i];
        int v2 = vertexIndices[(i + 1) % vertexIndices.size()];
        int edgeIndex = find_or_create_edge(v1, v2);

        faces[faceIndex].edges.push_back(edgeIndex);
        if (edges[edgeIndex].face1 == -1) {
            edges[edgeIndex].face1 = faceIndex;
        } else {
            edges[edgeIndex].face2 = faceIndex;
        }
    }
}

int Mesh::find_or_create_edge(int v1, int v2) {
    if (v1 > v2) std::swap(v1, v2);
    auto edgeKey = std::make_pair(v1, v2);
    auto it = edgeMap.find(edgeKey);
    if (it != edgeMap.end()) {
        return it->second;
    } else {
        edges.emplace_back(v1, v2);
        int edgeIndex = edges.size() - 1;
        edgeMap[edgeKey] = edgeIndex;
        return edgeIndex;
    }
}

void Mesh::gen_normals() {
    // Initialize normals to zero
    for (auto& vertex : vertices) {
        vertex.normal = glm::vec3(0.0f);
    }

    // Calculate face normals and add them to the vertex normals
    for (const auto& face : faces) {
        if (face.vertices.size() < 3) continue; // Skip degenerate faces

        glm::vec3 v0 = vertices[face.vertices[0]].position;
        glm::vec3 v1 = vertices[face.vertices[1]].position;
        glm::vec3 v2 = vertices[face.vertices[2]].position;

        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

        for (int vertexIndex : face.vertices) {
            vertices[vertexIndex].normal += normal;
        }
    }

    // Normalize the vertex normals
    for (auto& vertex : vertices) {
        vertex.normal = glm::normalize(vertex.normal);
    }

    has_normals = true;
}

void Mesh::upload() {
    if (uploaded) {
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glDeleteVertexArrays(1, &VAO);
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    std::vector<unsigned int> indices;
    for (const auto& face : faces) {
        indices.insert(indices.end(), face.vertices.begin(), face.vertices.end());
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
    uploaded = true;
}

void Mesh::to_obj(const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return;
    }

    for (const auto& vertex : vertices) {
        file << "v " << vertex.position.x << " " << vertex.position.y << " " << vertex.position.z << std::endl;
    }

    for (const auto& face : faces) {
        file << "f";
        for (const auto& index : face.vertices) {
            file << " " << (index + 1); // OBJ format is 1-based
        }
        file << std::endl;
    }

    file.close();
}

void Mesh::to_objq(const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return;
    }

    for (const auto& vertex : vertices) {
        file << "v " << vertex.position.x << " " << vertex.position.y << " " << vertex.position.z << std::endl;
    }

    for (const auto& face : faces) {
        if (face.vertices.size() == 4) {
            file << "f";
            for (const auto& index : face.vertices) {
                file << " " << (index + 1); // OBJ format is 1-based
            }
            file << std::endl;
        }
    }

    file.close();
}

Mesh Mesh::makeCube() {
    Mesh mesh;

    // Front face
    mesh.add_vertex(glm::vec3(-0.5f, -0.5f, 0.5f));
    mesh.add_vertex(glm::vec3(0.5f, -0.5f, 0.5f));
    mesh.add_vertex(glm::vec3(0.5f, 0.5f, 0.5f));
    mesh.add_vertex(glm::vec3(-0.5f, 0.5f, 0.5f));

    // Back face
    mesh.add_vertex(glm::vec3(-0.5f, -0.5f, -0.5f));
    mesh.add_vertex(glm::vec3(0.5f, -0.5f, -0.5f));
    mesh.add_vertex(glm::vec3(0.5f, 0.5f, -0.5f));
    mesh.add_vertex(glm::vec3(-0.5f, 0.5f, -0.5f));

    // Add faces
    mesh.add_face({0, 1, 2, 3}); // Front face
    mesh.add_face({7, 6, 5, 4}); // Back face
    mesh.add_face({0, 4, 5, 1}); // Bottom face
    mesh.add_face({3, 2, 6, 7}); // Top face
    mesh.add_face({1, 5, 6, 2}); // Right face
    mesh.add_face({4, 0, 3, 7}); // Left face

    return mesh;
}

#endif // MESH_H
