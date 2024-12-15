#include <unordered_map>
#include <memory>
#include <vector>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>

#include "mesh.h"

struct Vertex;
struct Face;

struct HalfEdge {
    std::shared_ptr<Vertex> origin;
    std::shared_ptr<HalfEdge> twin;
    std::shared_ptr<HalfEdge> next;
    std::shared_ptr<Face> face;
};

struct Face {
    std::shared_ptr<HalfEdge> edge;
};

struct Vertex {
    glm::vec3 position;
    std::shared_ptr<HalfEdge> edge;
};

struct ProcMesh
{
    std::vector<std::shared_ptr<Vertex>> vertices;
    std::vector<std::shared_ptr<Face>> faces;
    std::vector<std::shared_ptr<HalfEdge>> edges;

    static ProcMesh from_rendermesh(const RenderMesh &render_mesh)
    {
        ProcMesh proc_mesh;

        // Mapping from RenderMesh vertex indices to ProcMesh vertices
        std::vector<std::shared_ptr<Vertex>> vertex_map(render_mesh.positions.size());

        // Create ProcMesh vertices
        for (size_t i = 0; i < render_mesh.positions.size(); ++i)
        {
            const glm::vec3 &pos = render_mesh.positions[i];
            vertex_map[i] = std::make_shared<Vertex>(pos.x, pos.y, pos.z);
            proc_mesh.vertices.push_back(vertex_map[i]);
        }

        // Create ProcMesh faces and half-edges
        for (size_t i = 0; i < render_mesh.indices.size(); i += 3)
        {
            unsigned int i0 = render_mesh.indices[i];
            unsigned int i1 = render_mesh.indices[i + 1];
            unsigned int i2 = render_mesh.indices[i + 2];

            // Create face
            auto face = std::make_shared<Face>();
            proc_mesh.faces.push_back(face);

            // Create half-edges
            auto he0 = std::make_shared<HalfEdge>();
            auto he1 = std::make_shared<HalfEdge>();
            auto he2 = std::make_shared<HalfEdge>();

            // Link half-edges
            he0->next = he1;
            he1->next = he2;
            he2->next = he0;

            // Set origins
            he0->origin = vertex_map[i0];
            he1->origin = vertex_map[i1];
            he2->origin = vertex_map[i2];

            // Set face
            he0->face = face;
            he1->face = face;
            he2->face = face;

            // Link to the face
            face->edge = he0;

            // Store half-edges
            proc_mesh.edges.push_back(he0);
            proc_mesh.edges.push_back(he1);
            proc_mesh.edges.push_back(he2);

            // Assign half-edges to vertices
            vertex_map[i0]->edge = he0;
            vertex_map[i1]->edge = he1;
            vertex_map[i2]->edge = he2;
        }

        // Set twin relationships
        std::unordered_map<std::pair<int, int>, std::shared_ptr<HalfEdge>, std::hash<std::pair<int, int>>> edge_map;
        for (auto &edge : proc_mesh.edges)
        {
            auto origin_index = std::distance(proc_mesh.vertices.begin(),
                                              std::find(proc_mesh.vertices.begin(), proc_mesh.vertices.end(), edge->origin));
            auto destination_index = std::distance(proc_mesh.vertices.begin(),
                                                   std::find(proc_mesh.vertices.begin(), proc_mesh.vertices.end(), edge->next->origin));

            auto key = std::make_pair(origin_index, destination_index);
            auto reverse_key = std::make_pair(destination_index, origin_index);

            if (edge_map.find(reverse_key) != edge_map.end())
            {
                edge->twin = edge_map[reverse_key];
                edge_map[reverse_key]->twin = edge;
            }
            else
            {
                edge_map[key] = edge;
            }
        }

        return proc_mesh;
    }

    void ProcMesh::to_obj(const std::string &filename)
    {
        std::ofstream file(filename);

        // Write vertices
        for (const auto &vertex : vertices)
        {
            file << "v " << vertex->x << " " << vertex->y << " " << vertex->z << std::endl;
        }

        // Write faces
        for (const auto &face : faces)
        {
            file << "f";
            auto edge = face->edge;
            do
            {
                auto origin = edge->origin;
                auto it = std::find(vertices.begin(), vertices.end(), origin);
                int index = std::distance(vertices.begin(), it) + 1;
                file << " " << index;
                edge = edge->next;
            } while (edge != face->edge);
            file << std::endl;
        }

        file.close();
    }
};
