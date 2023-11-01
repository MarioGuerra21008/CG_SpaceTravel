#include <vector>
#include <SDL.h>
#include "glm/glm.hpp"
#include "loadOBJFile.h"
#pragma once

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 original;
    double z;
};

std::vector<Vertex> setupVertexArray(const std::vector<glm::vec3>& vertices, const std::vector<glm::vec3>& normals, const std::vector<Face>& faces) {

    std::vector<Vertex> vertexArray;
    for (const auto& face : faces)
    {
        glm::vec3 vertexPosition1 = vertices[face.vertexIndices[0]];
        glm::vec3 vertexPosition2 = vertices[face.vertexIndices[1]];
        glm::vec3 vertexPosition3 = vertices[face.vertexIndices[2]];
        glm::vec3 normalPosition1 = normals[face.normalIndices[0]];
        glm::vec3 normalPosition2 = normals[face.normalIndices[1]];
        glm::vec3 normalPosition3 = normals[face.normalIndices[2]];

        vertexArray.push_back(Vertex {vertexPosition1, normalPosition1});
        vertexArray.push_back(Vertex {vertexPosition2, normalPosition2});
        vertexArray.push_back(Vertex {vertexPosition3, normalPosition3});
    }
    return vertexArray;
}
