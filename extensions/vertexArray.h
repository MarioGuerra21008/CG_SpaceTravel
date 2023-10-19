#include <SDL.h>
#include <vector>
#include "glm/glm.hpp"
#include "fragment.h"
#include "loadOBJFile.h"

#pragma once

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tex;
    glm::vec3 original;
};

std::vector<Vertex> setupVertexArray(const std::vector<glm::vec3>& vertices, const std::vector<glm::vec3>& normals, const std::vector<Face>& faces)
{
    std::vector<Vertex> vertexArray;

    // Ajusta esta escala manualmente para obtener el tamaño deseado del modelo en la ventana
    float scale = 1.0f;

    // For each face
    for (const auto& face : faces)
    {
        // Get the vertex positions from the input arrays using the indices from the face
        glm::vec3 vertexPosition1 = vertices[face.vertexIndices[0]];
        glm::vec3 vertexPosition2 = vertices[face.vertexIndices[1]];
        glm::vec3 vertexPosition3 = vertices[face.vertexIndices[2]];

        glm::vec3 normalPosition1 = normals[face.normalIndices[0]];
        glm::vec3 normalPosition2 = normals[face.normalIndices[1]];
        glm::vec3 normalPosition3 = normals[face.normalIndices[2]];

        // Add the vertex positions to the vertex array
        vertexArray.push_back(Vertex {vertexPosition1, normalPosition1});
        vertexArray.push_back(Vertex {vertexPosition2, normalPosition2});
        vertexArray.push_back(Vertex {vertexPosition3, normalPosition3});


    }

    return vertexArray;
}