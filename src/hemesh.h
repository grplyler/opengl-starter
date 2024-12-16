#pragma once
#include <vector>
#include <unordered_map>
#include "mesh.h"

class Vertex;
class HalfEdge;
class Face;

class Vertex
{
public:
    float x, y, z;
    HalfEdge *outgoing_halfedge;

    Vertex(float x = 0, float y = 0, float z = 0)
        : x(x), y(y), z(z), outgoing_halfedge(nullptr) {}
};

class HalfEdge
{
public:
    Vertex *vertex;
    HalfEdge *opposite;
    HalfEdge *next;
    Face *face;

    HalfEdge()
        : vertex(nullptr), opposite(nullptr), next(nullptr), face(nullptr) {}
};

class Face
{
public:
    HalfEdge *halfedge; // One of the halfedges bordering this face

    Face() : halfedge(nullptr) {}
};

class HalfEdgeMesh
{
private:
    std::vector<std::unique_ptr<Vertex>> vertices;
    std::vector<std::unique_ptr<HalfEdge>> halfedges;
    std::vector<std::unique_ptr<Face>> faces;

public:
    Vertex *add_vertex(float x, float y, float z)
    {
        vertices.push_back(std::make_unique<Vertex>(x, y, z));
        return vertices.back().get();
    };

    Face *add_face(const std::vector<Vertex *> &input_vertices)
    {
        if (input_vertices.size() < 3)
        {
            throw std::invalid_argument("Face must have at least 3 vertices");
        }

        // Reverse vertices for counter-clockwise winding
        std::vector<Vertex *> face_vertices(input_vertices.rbegin(), input_vertices.rend());

        // Create a new face
        auto face = std::make_unique<Face>();
        Face *face_ptr = face.get();

        // Create halfedges for this face
        std::vector<HalfEdge *> new_halfedges;
        for (size_t i = 0; i < face_vertices.size(); ++i)
        {
            auto he = std::make_unique<HalfEdge>();
            he->vertex = face_vertices[(i + 1) % face_vertices.size()];
            he->face = face_ptr;

            if (!new_halfedges.empty())
            {
                new_halfedges.back()->next = he.get();
            }

            new_halfedges.push_back(he.get());
            halfedges.push_back(std::move(he));
        }

        // Connect the last halfedge to the first
        new_halfedges.back()->next = new_halfedges.front();

        // Set face's halfedge
        face->halfedge = new_halfedges.front();

        // Set vertices' outgoing halfedges
        for (size_t i = 0; i < face_vertices.size(); ++i)
        {
            face_vertices[i]->outgoing_halfedge = new_halfedges[i];
        }

        faces.push_back(std::move(face));
        return face_ptr;
    }

    void connect_opposites(HalfEdge *he1, HalfEdge *he2)
    {
        he1->opposite = he2;
        he2->opposite = he1;
    }

    std::vector<Vertex *> get_vertices_around_face(Face *face)
    {
        std::vector<Vertex *> result;
        if (!face || !face->halfedge)
            return result;

        HalfEdge *start = face->halfedge;
        HalfEdge *current = start;
        do
        {
            result.push_back(current->vertex);
            current = current->next;
        } while (current != start);

        return result;
    }

    std::vector<Vertex *> get_vertices_around_vertex(Vertex *vertex)
    {
        std::vector<Vertex *> result;
        if (!vertex || !vertex->outgoing_halfedge)
            return result;

        HalfEdge *start = vertex->outgoing_halfedge;
        HalfEdge *current = start;
        do
        {
            result.push_back(current->vertex);
            if (!current->opposite)
                break;
            current = current->opposite->next;
        } while (current != start);

        return result;
    }

    RenderMesh to_rendermesh()
    {
        RenderMesh mesh;

        // First create mapping from Vertex* to index
        std::unordered_map<Vertex *, size_t> vertex_indices;
        for (size_t i = 0; i < vertices.size(); i++)
        {
            vertex_indices[vertices[i].get()] = i;
            mesh.add_vertex(vertices[i]->x, vertices[i]->y, vertices[i]->z);
        }

        // Add faces using the vertex indices
        for (const auto &face : faces)
        {
            std::vector<Vertex *> face_vertices = get_vertices_around_face(face.get());

            // Triangulate face using fan triangulation
            for (size_t i = 1; i < face_vertices.size() - 1; i++)
            {
                mesh.add_face(
                    vertex_indices[face_vertices[0]],
                    vertex_indices[face_vertices[i]],
                    vertex_indices[face_vertices[i + 1]]);
            }
        }

        return mesh;
    }

    static HalfEdgeMesh from_rendermesh(const RenderMesh &render_mesh)
    {
        HalfEdgeMesh hemesh;

        // Create vertices
        std::vector<Vertex *> vertex_ptrs;
        for (const auto &pos : render_mesh.positions)
        {
            vertex_ptrs.push_back(hemesh.add_vertex(pos.x, pos.y, pos.z));
        }

        // Create faces from triangles with reversed winding
        for (size_t i = 0; i < render_mesh.indices.size(); i += 3)
        {
            std::vector<Vertex *> face_vertices = {
                vertex_ptrs[render_mesh.indices[i + 2]], // Reversed order
                vertex_ptrs[render_mesh.indices[i + 1]],
                vertex_ptrs[render_mesh.indices[i]]};
            hemesh.add_face(face_vertices);
        }

        return hemesh;
    }

    // Add pair_hash struct at class scope
    struct pair_hash {
        template <class T1, class T2>
        std::size_t operator()(const std::pair<T1, T2>& p) const {
            auto h1 = std::hash<T1>{}(p.first);
            auto h2 = std::hash<T2>{}(p.second);
            return h1 ^ (h2 << 1);
        }
    };

    HalfEdgeMesh subdivide_loop() {
        HalfEdgeMesh new_mesh;
        std::unordered_map<Vertex*, Vertex*> old_to_new_vertices;
        std::unordered_map<std::pair<Vertex*, Vertex*>, Vertex*, pair_hash> edge_to_vertex;

        // Step 1: Calculate new vertex positions (3/8 * old + 5/8 * neighbor average)
        for (const auto& v : vertices) {
            auto neighbors = get_vertices_around_vertex(v.get());
            int n = neighbors.size();
            
            float beta = (n == 3) ? 3.0f/16.0f : 3.0f/(8.0f * n);
            glm::vec3 sum(0.0f);
            for (auto neighbor : neighbors) {
                sum += glm::vec3(neighbor->x, neighbor->y, neighbor->z);
            }
            
            glm::vec3 new_pos = glm::vec3(v->x, v->y, v->z) * (1.0f - n * beta) + sum * beta;
            old_to_new_vertices[v.get()] = new_mesh.add_vertex(new_pos.x, new_pos.y, new_pos.z);
        }

        // Step 2: Create edge vertices using the 3/8 - 3/8 - 1/8 - 1/8 rule
        for (const auto& face : faces) {
            HalfEdge* start = face->halfedge;
            HalfEdge* current = start;
            do {
                Vertex* v0 = current->vertex;
                Vertex* v1 = current->next->vertex;
                auto edge = v0 < v1 ? 
                    std::make_pair(v0, v1) : 
                    std::make_pair(v1, v0);

                if (edge_to_vertex.find(edge) == edge_to_vertex.end()) {
                    glm::vec3 edge_point;
                    if (current->opposite) {
                        // Interior edge: use 3/8 - 3/8 - 1/8 - 1/8 rule
                        edge_point = 
                            glm::vec3(v0->x, v0->y, v0->z) * 0.375f +
                            glm::vec3(v1->x, v1->y, v1->z) * 0.375f +
                            glm::vec3(current->next->next->vertex->x,
                                     current->next->next->vertex->y,
                                     current->next->next->vertex->z) * 0.125f +
                            glm::vec3(current->opposite->next->next->vertex->x,
                                     current->opposite->next->next->vertex->y,
                                     current->opposite->next->next->vertex->z) * 0.125f;
                    } else {
                        // Boundary edge: simple average
                        edge_point = (glm::vec3(v0->x, v0->y, v0->z) +
                                    glm::vec3(v1->x, v1->y, v1->z)) * 0.5f;
                    }
                    edge_to_vertex[edge] = new_mesh.add_vertex(
                        edge_point.x, edge_point.y, edge_point.z);
                }
                current = current->next;
            } while (current != start);
        }

        // Step 3: Create new faces with correct winding
        for (const auto& face : faces) {
            HalfEdge* start = face->halfedge;
            
            Vertex* v0 = old_to_new_vertices[start->vertex];
            Vertex* v1 = old_to_new_vertices[start->next->vertex];
            Vertex* v2 = old_to_new_vertices[start->next->next->vertex];

            auto e0 = edge_to_vertex[std::make_pair(
                std::min(start->vertex, start->next->vertex),
                std::max(start->vertex, start->next->vertex))];
            auto e1 = edge_to_vertex[std::make_pair(
                std::min(start->next->vertex, start->next->next->vertex),
                std::max(start->next->vertex, start->next->next->vertex))];
            auto e2 = edge_to_vertex[std::make_pair(
                std::min(start->next->next->vertex, start->vertex),
                std::max(start->next->next->vertex, start->vertex))];

            // Create four triangles with consistent winding
            new_mesh.add_face({v0, e0, e2});
            new_mesh.add_face({e0, v1, e1});
            new_mesh.add_face({e2, e1, v2});
            new_mesh.add_face({e0, e1, e2});
        }

        return new_mesh;
    }
};