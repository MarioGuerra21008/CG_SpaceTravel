#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <array>
#include <cmath>
#include <random>
#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "extensions/color.h"
#include "extensions/framebuffer.h"
#include "extensions/point.h"
#include "extensions/line.h"
#include "extensions/triangle.h"
#include "extensions/fragment.h"
#include "extensions/uniform.h"
#include "extensions/shaders.h"
#include "extensions/vertexArray.h"
#include "extensions/loadOBJFile.h"
#include "extensions/FastNoiseLite.h"

const int WINDOW_WIDTH = 1080;
const int WINDOW_HEIGHT = 720;
float pi = 3.14f / 3.0f;

Color colorClear = {0, 0, 0, 255};
Color current = {255, 255, 255, 255};
Color color1 = {255, 0, 0, 255};
Color color2 = {0, 255, 0, 255};
Color color3 = {0, 0, 255, 255};

glm::vec3 light = glm::vec3(0.0f, 0.0f, 0.0f);

SDL_Window* window;
Uniform uniform;

std::array<double, SCREEN_WIDTH * SCREEN_HEIGHT> zBuffer;

struct Camera {
    glm::vec3 cameraPosition;
    glm::vec3 targetPosition;
    glm::vec3 upVector;
};

void clear(SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderClear(renderer);
}

Color interpolateColor(const glm::vec3& barycentricCoord, const Color& color1, const Color& color2, const Color& color3) {
    float u = barycentricCoord.x;
    float v = barycentricCoord.y;
    float w = barycentricCoord.z;

    // Realiza una interpolación lineal para cada componente del color
    uint8_t r = static_cast<uint8_t>(u * color1.r + v * color2.r + w * color3.r);
    uint8_t g = static_cast<uint8_t>(u * color1.g + v * color2.g + w * color3.g);
    uint8_t b = static_cast<uint8_t>(u * color1.b + v * color2.b + w * color3.b);
    uint8_t a = static_cast<uint8_t>(u * color1.a + v * color2.a + w * color3.a);

    return Color(r, g, b, a);
}

bool isBarycentricCoord(const glm::vec3& barycentricCoord) {
    return barycentricCoord.x >= 0 && barycentricCoord.y >= 0 && barycentricCoord.z >= 0 &&
           barycentricCoord.x <= 1 && barycentricCoord.y <= 1 && barycentricCoord.z <= 1 &&
           glm::abs(1 - (barycentricCoord.x + barycentricCoord.y + barycentricCoord.z)) < 0.00001f;
}

glm::vec3 calculateBarycentricCoord(const glm::vec2& A, const glm::vec2& B, const glm::vec2& C, const glm::vec2& P) {
    float denominator = (B.y - C.y) * (A.x - C.x) + (C.x - B.x) * (A.y - C.y);
    float u = ((B.y - C.y) * (P.x - C.x) + (C.x - B.x) * (P.y - C.y)) / denominator;
    float v = ((C.y - A.y) * (P.x - C.x) + (A.x - C.x) * (P.y - C.y)) / denominator;
    float w = 1 - u - v;
    return glm::vec3(u, v, w);
}

std::vector<Fragment> triangle(const Vertex& a, const Vertex& b, const Vertex& c) {
    std::vector<Fragment> fragments;

    // Calculate the bounding box of the triangle
    int minX = static_cast<int>(std::min({a.position.x, b.position.x, c.position.x}));
    int minY = static_cast<int>(std::min({a.position.y, b.position.y, c.position.y}));
    int maxX = static_cast<int>(std::max({a.position.x, b.position.x, c.position.x}));
    int maxY = static_cast<int>(std::max({a.position.y, b.position.y, c.position.y}));

    // Iterate over each point in the bounding box
    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            glm::vec2 pixelPosition(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f); // Central point of the pixel
            glm::vec3 barycentricCoord = calculateBarycentricCoord(a.position, b.position, c.position, pixelPosition);

            if (isBarycentricCoord(barycentricCoord)) {
                Color p {0, 0, 0};
                // Interpolate attributes (color, depth, etc.) using barycentric coordinates
                Color interpolatedColor = interpolateColor(barycentricCoord, p, p, p);

                // Calculate the interpolated Z value using barycentric coordinates
                float interpolatedZ = barycentricCoord.x * a.position.z + barycentricCoord.y * b.position.z + barycentricCoord.z * c.position.z;

                // Create a fragment with the position, interpolated attributes, and Z coordinate
                Fragment fragment;
                fragment.position = glm::ivec2(x, y);
                fragment.color = interpolatedColor;
                fragment.z = interpolatedZ;

                fragments.push_back(fragment);
            }
        }
    }

    return fragments;
}

std::vector<std::vector<glm::vec3>> primitiveAssembly(const std::vector<glm::vec3>& transformedVertices) {
    std::vector<std::vector<glm::vec3>> groupedVertices;

    for (int i = 0; i < transformedVertices.size(); i += 3) {
        std::vector<glm::vec3> triangle;
        triangle.push_back(transformedVertices[i]);
        triangle.push_back(transformedVertices[i+1]);
        triangle.push_back(transformedVertices[i+2]);

        groupedVertices.push_back(triangle);
    }

    return groupedVertices;
}

glm::mat4 createModelMatrix() {
    glm::mat4 translation = glm::translate(glm::mat4(1), glm::vec3(0.0f, 0.0f, 0.0f));
    glm::mat4 scale = glm::scale(glm::mat4(1), glm::vec3(1.0f, 1.0f, 1.0f));
    glm::mat4 rotation = glm::rotate(glm::mat4(1), glm::radians((pi++)), glm::vec3(0.0f, 1.0f, 0.0f));
    return translation * scale * rotation;
}

glm::mat4 createViewMatrix() {
    return glm::lookAt(
            // En donde se encuentra
            glm::vec3(0, 0, 2),
            // En donde está viendo
            glm::vec3(0, 0, 0),
            // Hacia arriba para la cámara
            glm::vec3(0, 1, 0)
    );
}

glm::mat4 createProjectionMatrix() {
    float fovInDegrees = 45.0f;
    float aspectRatio = SCREEN_WIDTH / SCREEN_HEIGHT;
    float nearClip = 0.1f;
    float farClip = 100.0f;

    return glm::perspective(glm::radians(fovInDegrees), aspectRatio, nearClip, farClip);
}

glm::mat4 createViewportMatrix() {
    glm::mat4 viewport = glm::mat4(1.0f);
    // Scale
    viewport = glm::scale(viewport, glm::vec3(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 1.5f, 0.5f));
    // Translate
    viewport = glm::translate(viewport, glm::vec3(1.0f, 0.75f, 1.0f));

    return viewport;
}

void render(const std::vector<Vertex>& vertexArray,  const Uniform& uniform) {
    std::vector<Vertex> transformedVertexArray;
    for (const auto& vertex : vertexArray) {
        auto transformedVertex = vertexShader(vertex, uniform);
        transformedVertexArray.push_back(transformedVertex);
    }

    for (size_t i = 0; i < transformedVertexArray.size(); i += 3) {
        const Vertex& a = transformedVertexArray[i];
        const Vertex& b = transformedVertexArray[i + 1];
        const Vertex& c = transformedVertexArray[i + 2];

        glm::vec3 A = a.position;
        glm::vec3 B = b.position;
        glm::vec3 C = c.position;

        // Bounding box para el triangulo
        int minX = static_cast<int>(std::min({A.x, B.x, C.x}));
        int minY = static_cast<int>(std::min({A.y, B.y, C.y}));
        int maxX = static_cast<int>(std::max({A.x, B.x, C.x}));
        int maxY = static_cast<int>(std::max({A.y, B.y, C.y}));

        // Iterating
        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                if (y>0 && y<SCREEN_HEIGHT && x>0 && x<SCREEN_WIDTH) {
                    glm::vec2 pixelPosition(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f); // Central point of the pixel
                    glm::vec3 barycentricCoord = calculateBarycentricCoord(A, B, C, pixelPosition);

                    if (isBarycentricCoord(barycentricCoord)) {
                        Color modelColor {0, 0, 0};
                        Color interpolatedColor = interpolateColor(barycentricCoord, modelColor, modelColor, modelColor);

                        float depth = barycentricCoord.x * A.z + barycentricCoord.y * B.z + barycentricCoord.z * C.z;

                        glm::vec3 normal = a.normal * barycentricCoord.x + b.normal * barycentricCoord.y+ c.normal * barycentricCoord.z;

                        float fragmentIntensity = glm::dot(normal, glm::vec3 (0.0f,0.0f,1.0f));
                        if (fragmentIntensity <= 0){
                            continue;
                        }

                        // Apply fragment shader to calculate final color with shading
                        Color finalColor = interpolatedColor * fragmentIntensity;
                        glm::vec3 original = a.original * barycentricCoord.x + b.original * barycentricCoord.y + c.original * barycentricCoord.z;
                        // Create a fragment with the position, interpolated attributes, and depth
                        Fragment fragment;
                        fragment.position = glm::ivec2(x, y);
                        fragment.color = finalColor;
                        fragment.z = depth;
                        fragment.original = original;

                        int index = y * WINDOW_WIDTH + x;
                        if (depth < zBuffer[index]) {
                            // Apply fragment shader to calculate final color
                            Color fragmentShaderf = fragmentShaderEarth(fragment); //Cambiar nombre del método para ver los shaders
                            // Draw the fragment using SDL_SetRenderDrawColor and SDL_RenderDrawPoint
                            SDL_SetRenderDrawColor(renderer, fragmentShaderf.r, fragmentShaderf.g, fragmentShaderf.b, fragmentShaderf.a);
                            SDL_RenderDrawPoint(renderer, x, WINDOW_HEIGHT-y);
                            nextTime = 0.5f + 1.0f;
                            zBuffer[index] = depth;
                        }
                    }
                }
            }
        }
    }
}

int main(int argc, char* args[]) {
    SDL_Init(SDL_INIT_EVERYTHING);
    glm::vec3 translation(0.0f, 0.0f, 0.0f); // Definir la posición del objeto en el mundo
    glm::vec3 rotationAngles(0.0f, 0.0f, 0.0f); // Ángulos de rotación en los ejes X, Y y Z (en grados)
    glm::vec3 scale(1.0f, 1.0f, 1.0f);

    glm::vec3 cameraPosition(0.0f, 0.0f, 0.0f); // Mueve la cámara hacia atrás
    glm::vec3 targetPosition(0.0f, 0.0f, 0.0f);   // Coloca el centro de la escena en el origen
    glm::vec3 upVector(0.0f, 1.0f, 0.0f);

    uniform.view = createViewMatrix();

    srand(time(nullptr));

    window = SDL_CreateWindow("SR", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    int renderWidth, renderHeight;
    SDL_GetRendererOutputSize(renderer, &renderWidth, &renderHeight);

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normal;
    std::vector<Face> faces;

    bool success = loadOBJ("../models/sphere.obj", vertices, normal, faces);
    if (!success) {
        return 1;
    }

    std::vector<Vertex> vertexArray = setupVertexArray(vertices, normal, faces);

    bool running = true;
    SDL_Event event;
    glm::mat4 rotationMatrix = glm::mat4(1.0f); // Inicializa la matriz de rotación
    glm::mat4 traslateMatrix = glm::mat4(1.0f); // Inicializa la matriz de traslación
    glm::mat4 scaleMatrix = glm::mat4(1.0f); // Inicializa la matriz de escala

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        uniform.model = createModelMatrix();
        uniform.projection = createProjectionMatrix();
        uniform.viewport = createViewportMatrix();

        SDL_SetRenderDrawColor(renderer, colorClear.r, colorClear.g, colorClear.b, colorClear.a);
        SDL_RenderClear(renderer);

        glm::vec4 transformedLight = glm::inverse(createModelMatrix()) * glm::vec4(light, 0.0f);
        glm::vec3 transformedLightDirection = glm::normalize(glm::vec3(transformedLight));
        std::fill(zBuffer.begin(), zBuffer.end(), std::numeric_limits<double>::max());
        // Llamada a la función render con la matriz de vértices transformados
        render(vertexArray, uniform);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
