#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <array>
#include <cmath>
#include <random>
#include <mutex>
#include <thread>
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
std::mutex mutex;

//FrameRate
Uint32 startingFrame; // Tiempo de inicio del cuadro actual
Uint32 frameTime; // Tiempo transcurrido en el cuadro actual
int frameCounter = 0;  // Contador
int fps = 0; // FPS

Color colorClear = {0, 0, 0, 255};
Color current = {255, 255, 255, 255};
Color color1 = {255, 0, 0, 255};
Color color2 = {0, 255, 0, 255};
Color color3 = {0, 0, 255, 255};

glm::vec3 light = glm::vec3(0.0f, 0.0f, 200.0f);

SDL_Window* window;
Uniform uniform;

std::array<double, SCREEN_WIDTH * SCREEN_HEIGHT> zBuffer;

enum Planets {
    SPACE,
    SUN,
    EARTH,
    MARS,
    JUPITER,
    SATURN,
    URANUS,
    NEPTUNE,
    SHIP
};

struct BuildingModel {
    Uniform uniform;
    std::vector<Vertex>* v;
    Planets i;
};

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
    float aspectRatio = WINDOW_WIDTH / SCREEN_HEIGHT;
    float nearClip = 0.1f;
    float farClip = 100.0f;

    return glm::perspective(glm::radians(fovInDegrees), aspectRatio, nearClip, farClip);
}

glm::mat4 createViewportMatrix() {
    glm::mat4 viewport = glm::mat4(1.0f);
    // Scale
    viewport = glm::scale(viewport, glm::vec3(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 1.5f, 0.5f));
    // Translate
    viewport = glm::translate(viewport, glm::vec3(1.0f, 1.0f, 1.5f));

    return viewport;
}

glm::mat4 createModelSpace() {
    glm::mat4 translation = glm::translate(glm::mat4(1), glm::vec3(0.0f, 0.0f, -30.0f));
    glm::mat4 scale = glm::scale(glm::mat4(1), glm::vec3(20.0f, 20.0f, 20.0f));
    glm::mat4 rotation = glm::mat4(1.0f);
    return translation * scale * rotation;
}

glm::mat4 createModelSpaceship(glm::vec3 cameraPosition, glm::vec3 targetPosition,glm::vec3 upVector, float rotX, float rotY) {
    glm::mat4 translation = glm::translate(glm::mat4(1), (targetPosition - cameraPosition) / 7.0f + cameraPosition - upVector *0.15f);
    glm::mat4 scale = glm::scale(glm::mat4(1), glm::vec3(0.1f, 0.1f, 0.1f));
    glm::mat4 rotationX = glm::rotate(glm::mat4(1), glm::radians(-rotX - 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 rotationY = glm::rotate(glm::mat4(1), glm::radians(-rotY), glm::vec3(0.0f, 0.0f, 1.0f));
    return translation * scale * rotationX * rotationY;
}

glm::mat4 createModelPlanet(glm::vec3 translationM, glm::vec3 scaleM, glm::vec3 rotationM, float radianSpeed)  {
    glm::mat4 translation = glm::translate(glm::mat4(1), glm::vec3(0.0f, 0.0f, 0.0f));
    glm::mat4 scale = glm::scale(glm::mat4(1), glm::vec3(1.0f, 1.0f, 1.0f));
    glm::mat4 rotation = glm::rotate(glm::mat4(1), glm::radians((pi++)), glm::vec3(0.0f, 1.0f, 0.0f));
    return translation * scale * rotation;
}

glm::vec3 calculatePositionInCircle(float angleR, float radius){
    float xPosition = glm::cos(angleR) * radius;
    float zPosition = glm::sin(angleR) * radius;
    return glm::vec3(xPosition, 0.0f, zPosition);
}

// Declaración de variables para los shaders del sistema solar y la nave.

Uniform uniform1;
Uniform uniform2;
Uniform uniform3;
Uniform uniform4;
Uniform uniform5;
Uniform uniform6;
Uniform uniform7;
Uniform uniform8;
Uniform uniform9;

BuildingModel model1;
BuildingModel model2;
BuildingModel model3;
BuildingModel model4;
BuildingModel model5;
BuildingModel model6;
BuildingModel model7;
BuildingModel model8;
BuildingModel model9;

void render(const std::vector<Vertex>& vertexArray,  const Uniform& uniform, int planetIdentifier) {
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
                    glm::vec2 pixelPosition(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f);
                    glm::vec3 barycentricCoord = calculateBarycentricCoord(A, B, C, pixelPosition);

                    double cam = barycentricCoord.x * a.z + barycentricCoord.y * b.z + barycentricCoord.z * c.z;

                    if (isBarycentricCoord(barycentricCoord) && cam > 0) {
                        Color modelColor {0, 0, 0};
                        Color interpolatedColor = interpolateColor(barycentricCoord, modelColor, modelColor, modelColor);

                        float depth = barycentricCoord.x * A.z + barycentricCoord.y * B.z + barycentricCoord.z * C.z;

                        glm::vec3 normal = a.normal * barycentricCoord.x + b.normal * barycentricCoord.y+ c.normal * barycentricCoord.z;

                        float fragmentIntensity = (abs(glm::dot(normal, light)) > 1 ) ? 1: abs(glm::dot(normal, light));

                        if (planetIdentifier == SPACE) {
                            fragmentIntensity = glm::dot(normal, glm::vec3(0.0f,0.0f,1.0f));
                        }
                        if (fragmentIntensity <= 0){
                            continue;
                        }

                        Color finalColor = interpolatedColor * fragmentIntensity;
                        glm::vec3 original = a.original * barycentricCoord.x + b.original * barycentricCoord.y + c.original * barycentricCoord.z;

                        Fragment fragment;
                        fragment.position = glm::ivec2(x, y);
                        fragment.color = finalColor;
                        fragment.z = depth;
                        fragment.original = original;

                        int index = y * WINDOW_WIDTH + x;
                        if (depth < zBuffer[index]) {
                            mutex.lock();
                            Color fragmentShaderf;
                            switch(planetIdentifier) {
                                case SPACE:
                                    fragmentShaderf = fragmentShader(fragment);
                                    SDL_SetRenderDrawColor(renderer, fragmentShaderf.r, fragmentShaderf.g, fragmentShaderf.b, fragmentShaderf.a);
                                    break;
                                case SHIP:
                                    fragmentShaderf = fragmentShaderSpaceship(fragment);
                                    SDL_SetRenderDrawColor(renderer, fragmentShaderf.r, fragmentShaderf.g, fragmentShaderf.b, fragmentShaderf.a);
                                    break;
                                case SUN:
                                    fragmentShaderf = fragmentShaderSun(fragment);
                                    SDL_SetRenderDrawColor(renderer, fragmentShaderf.r, fragmentShaderf.g, fragmentShaderf.b, fragmentShaderf.a);
                                    break;
                                case EARTH:
                                    fragmentShaderf = fragmentShaderEarth(fragment);
                                    SDL_SetRenderDrawColor(renderer, fragmentShaderf.r, fragmentShaderf.g, fragmentShaderf.b, fragmentShaderf.a);
                                    break;
                                case MARS:
                                    fragmentShaderf = fragmentShaderMars(fragment);
                                    SDL_SetRenderDrawColor(renderer, fragmentShaderf.r, fragmentShaderf.g, fragmentShaderf.b, fragmentShaderf.a);
                                    break;
                                case JUPITER:
                                    fragmentShaderf = fragmentShaderJupiter(fragment);
                                    SDL_SetRenderDrawColor(renderer, fragmentShaderf.r, fragmentShaderf.g, fragmentShaderf.b, fragmentShaderf.a);
                                    break;
                                case SATURN:
                                    fragmentShaderf = fragmentShaderSaturn(fragment);
                                    SDL_SetRenderDrawColor(renderer, fragmentShaderf.r, fragmentShaderf.g, fragmentShaderf.b, fragmentShaderf.a);
                                    break;
                                case URANUS:
                                    fragmentShaderf = fragmentShaderUranus(fragment);
                                    SDL_SetRenderDrawColor(renderer, fragmentShaderf.r, fragmentShaderf.g, fragmentShaderf.b, fragmentShaderf.a);
                                    break;
                                case NEPTUNE:
                                    fragmentShaderf = fragmentShaderNeptune(fragment);
                                    SDL_SetRenderDrawColor(renderer, fragmentShaderf.r, fragmentShaderf.g, fragmentShaderf.b, fragmentShaderf.a);
                                    break;
                            }

                            SDL_RenderDrawPoint(renderer, x, WINDOW_HEIGHT-y);
                            nextTime = 0.5f + 1.0f;
                            zBuffer[index] = depth;
                            mutex.unlock();
                        }
                    }
                }
            }
        }
    }
}

int main(int argc, char* args[]) {
    SDL_Init(SDL_INIT_EVERYTHING);

    startingFrame = SDL_GetTicks();
    window = SDL_CreateWindow("Space Travel", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    std::vector<BuildingModel> models;
    int renderWidth, renderHeight;
    SDL_GetRendererOutputSize(renderer, &renderWidth, &renderHeight);

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normal;
    std::vector<Face> faces;

    bool success = loadOBJ("../models/sphere.obj", vertices, normal, faces);
    if (!success) {
        return 1;
    }

    std::vector<glm::vec3> spaceshipVertices;
    std::vector<glm::vec3> spaceshipNormal;
    std::vector<Face> spaceshipFaces;

    std::vector<Vertex> vertexArrayPlanet = setupVertexArray(vertices, normal, faces);

    bool success2 = loadOBJ("../models/cube.obj", spaceshipVertices, spaceshipNormal, spaceshipFaces);
    if (!success2) {
        return 1;
    }

    std::vector<Vertex> vertexArrayShip = setupVertexArray(spaceshipVertices, spaceshipNormal, spaceshipFaces);

    float forwardBackwardMovementSpeed = 0.1f;
    float leftRightMovementSpeed = 0.1f;
    float rotationX = 0.0f;
    float rotationY = 0.0f;
    float sunRotation = 0.0f;
    float earthRotation = 0.0f;
    float marsRotation = 0.0f;
    float jupiterRotation = 0.0f;
    float saturnRotation = 0.0f;
    float uranusRotation = 0.0f;
    float neptuneRotation = 0.0f;
    bool cameraToSystem = false;
    bool cameraToSun = false;
    bool cameraToEarth = false;
    bool cameraToMars = false;
    bool cameraToJupiter = false;
    bool cameraToSaturn = false;
    bool cameraToUranus = false;
    bool cameraToNeptune = false;

    glm::vec3 cameraPosition(0.0f, 0.0f, 5.0f); // Mueve la cámara hacia atrás
    glm::vec3 targetPosition(0.0f, 0.0f, 0.0f);   // Coloca el centro de la escena en el origen
    glm::vec3 upVector(0.0f, 1.0f, 0.0f);

    glm::vec3 newCameraPosition;
    glm::vec3 newSpaceshipCameraPosition;

    bool running = true;
    SDL_Event event;
    float speed = 0.01f;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            else if (event.type = SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_w:
                        cameraPosition -= forwardBackwardMovementSpeed * glm::normalize(targetPosition - cameraPosition);
                        targetPosition -= forwardBackwardMovementSpeed * (targetPosition - cameraPosition);
                        break;
                    case SDLK_a:
                        cameraPosition -= leftRightMovementSpeed * glm::normalize(glm::cross((targetPosition - cameraPosition), upVector))*5.0f;
                        targetPosition -= leftRightMovementSpeed * glm::normalize(glm::cross((targetPosition - cameraPosition), upVector))*5.0f;
                        break;
                    case SDLK_s:
                        cameraPosition += forwardBackwardMovementSpeed * glm::normalize(targetPosition - cameraPosition);
                        targetPosition += forwardBackwardMovementSpeed * (targetPosition - cameraPosition);
                        break;
                    case SDLK_d:
                        cameraPosition += leftRightMovementSpeed * glm::normalize(glm::cross((targetPosition - cameraPosition), upVector))*5.0f;
                        targetPosition += leftRightMovementSpeed * glm::normalize(glm::cross((targetPosition - cameraPosition), upVector))*5.0f;
                        break;
                    case SDLK_i:
                        rotationY -= 1.0f;
                        break;
                    case SDLK_j:
                        rotationX -= 1.0f;
                        break;
                    case SDLK_k:
                        rotationY += 1.0f;
                        break;
                    case SDLK_l:
                        rotationX += 1.0f;
                        break;
                    case SDLK_q:
                        cameraToSystem = !cameraToSystem;
                        break;
                    case SDLK_e:
                        cameraToSun = !cameraToSun;
                        break;
                    case SDLK_r:
                        cameraToEarth = !cameraToEarth;
                        break;
                    case SDLK_t:
                        cameraToMars = !cameraToMars;
                        break;
                    case SDLK_y:
                        cameraToJupiter = !cameraToJupiter;
                        break;
                    case SDLK_u:
                        cameraToSaturn = !cameraToSaturn;
                        break;
                    case SDLK_o:
                        cameraToUranus = !cameraToUranus;
                        break;
                    case SDLK_p:
                        cameraToNeptune = !cameraToNeptune;
                        break;
                }
            }
        }

        models.clear();
        light = cameraPosition - targetPosition;
        sunRotation += 0.01f;
        earthRotation += 0.2f;
        marsRotation += 0.15f;
        jupiterRotation += 0.07f;
        saturnRotation += 0.09f;
        uranusRotation += 0.1f;
        neptuneRotation += 0.085f;
        targetPosition = glm::vec3(10.0f * sin(glm::radians(rotationX)) * cos(glm::radians(rotationY)), 10.0f * sin(glm::radians(rotationY)), -10.0f * cos(glm::radians(rotationX)) * cos(glm::radians(rotationY))) + cameraPosition;

        glm::vec3 translateEarth = calculatePositionInCircle(earthRotation, 2.5f);
        glm::vec3 translateMars = calculatePositionInCircle(marsRotation, 3.4f);
        glm::vec3 translateJupiter = calculatePositionInCircle(jupiterRotation, 6.5f);
        glm::vec3 translateSaturn = calculatePositionInCircle(saturnRotation, 7.5f);
        glm::vec3 translateUranus = calculatePositionInCircle(uranusRotation, 8.5f);
        glm::vec3 translateNeptune = calculatePositionInCircle(neptuneRotation, 9.0f);

        uniform1.model = createModelSpace();
        uniform1.view = glm::lookAt(cameraPosition, targetPosition, upVector);
        uniform1.projection = createProjectionMatrix();
        uniform1.viewport = createViewportMatrix();

        model1.uniform = uniform1;
        model1.v = &vertexArrayPlanet;
        model1.i = SPACE;

        uniform2.model = createModelPlanet(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), 0.1f);
        uniform2.view =  glm::lookAt(cameraPosition, targetPosition, upVector);
        uniform2.projection = createProjectionMatrix();
        uniform2.viewport = createViewportMatrix();

        model2.uniform = uniform2;
        model2.v = &vertexArrayPlanet;
        model2.i = SUN;

        uniform3.model = createModelPlanet(translateEarth, glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(0.0f, 1.0f, 0.0f), 0.4f);
        uniform3.view = glm::lookAt(cameraPosition, targetPosition, upVector);
        uniform3.projection = createProjectionMatrix();
        uniform3.viewport = createViewportMatrix();

        model3.uniform = uniform3;
        model3.v = &vertexArrayPlanet;
        model3.i = EARTH;

        uniform4.model = createModelPlanet(translateMars, glm::vec3(0.4f, 0.4f, 0.4f), glm::vec3(0.0f, 1.0f, 0.0f), 0.3f);
        uniform4.view = glm::lookAt(cameraPosition, targetPosition, upVector);
        uniform4.projection = createProjectionMatrix();
        uniform4.viewport = createViewportMatrix();

        model4.uniform = uniform4;
        model4.v = &vertexArrayPlanet;
        model4.i = MARS;

        uniform5.model = createModelPlanet(translateJupiter, glm::vec3(0.9f, 0.9f, 0.9f), glm::vec3(0.0f, 1.0f, 0.0f), 0.15f);
        uniform5.view = glm::lookAt(cameraPosition, targetPosition, upVector);
        uniform5.projection = createProjectionMatrix();
        uniform5.viewport = createViewportMatrix();

        model5.uniform = uniform5;
        model5.v = &vertexArrayPlanet;
        model5.i = JUPITER;

        uniform6.model = createModelPlanet(translateSaturn, glm::vec3(0.7f, 0.7f, 0.7f), glm::vec3(0.0f, 1.0f, 0.0f), 0.2f);
        uniform6.view = glm::lookAt(cameraPosition, targetPosition, upVector);
        uniform6.projection = createProjectionMatrix();
        uniform6.viewport = createViewportMatrix();

        model6.uniform = uniform6;
        model6.v = &vertexArrayPlanet;
        model6.i = SATURN;

        uniform7.model = createModelPlanet(translateUranus, glm::vec3(0.6f, 0.6f, 0.6f), glm::vec3(0.0f, 1.0f, 0.0f), -0.2f);
        uniform7.view = glm::lookAt(cameraPosition, targetPosition, upVector);
        uniform7.projection = createProjectionMatrix();
        uniform7.viewport = createViewportMatrix();

        model7.uniform = uniform7;
        model7.v = &vertexArrayPlanet;
        model7.i = URANUS;

        uniform8.model = createModelPlanet(translateNeptune, glm::vec3(0.8f, 0.8f, 0.8f), glm::vec3(0.0f, 1.0f, 0.0f), 0.2f);
        uniform8.view = glm::lookAt(cameraPosition, targetPosition, upVector);
        uniform8.projection = createProjectionMatrix();
        uniform8.viewport = createViewportMatrix();

        model8.uniform = uniform8;
        model8.v = &vertexArrayPlanet;
        model8.i = NEPTUNE;

        uniform9.model = createModelSpaceship(cameraPosition, targetPosition, upVector, rotationX, rotationY);
        uniform9.view = glm::lookAt(cameraPosition, targetPosition, upVector);
        uniform9.projection = createProjectionMatrix();
        uniform9.viewport = createViewportMatrix();

        model9.uniform = uniform9;
        model9.v = &vertexArrayShip;
        model9.i = SHIP;

        if (cameraToSystem) {
            upVector = glm::vec3 (1.0f, 0.0f, 0.0f);
            cameraPosition = glm::vec3 (0,10,0);
            targetPosition = glm::vec3 (0, 0, 0);
        }

        if (cameraToEarth) {
            glm::vec3 orientationCamera = targetPosition - cameraPosition;
            targetPosition = translateEarth + orientationCamera * 0.75f;
            cameraPosition = targetPosition - orientationCamera;
        }

        if (cameraToMars) {
            glm::vec3 orientationCamera = targetPosition - cameraPosition;
            targetPosition = translateMars + orientationCamera * 0.75f;
            cameraPosition = targetPosition - orientationCamera;
        }

        if (cameraToJupiter) {
            glm::vec3 orientationCamera = targetPosition - cameraPosition;
            targetPosition = translateJupiter + orientationCamera * 0.75f;
            cameraPosition = targetPosition - orientationCamera;
        }

        if (cameraToSaturn) {
            glm::vec3 orientationCamera = targetPosition - cameraPosition;
            targetPosition = translateSaturn + orientationCamera * 0.75f;
            cameraPosition = targetPosition - orientationCamera;
        }

        if (cameraToUranus) {
            glm::vec3 orientationCamera = targetPosition - cameraPosition;
            targetPosition = translateUranus + orientationCamera * 0.75f;
            cameraPosition = targetPosition - orientationCamera;
        }

        if (cameraToNeptune) {
            glm::vec3 orientationCamera = targetPosition - cameraPosition;
            targetPosition = translateNeptune + orientationCamera * 0.75f;
            cameraPosition = targetPosition - orientationCamera;
        }

        //Pushback a todos los modelos excepto el del espacio.
        models.push_back(model2);
        models.push_back(model3);
        models.push_back(model4);
        models.push_back(model5);
        models.push_back(model6);
        models.push_back(model7);
        models.push_back(model8);
        models.push_back(model9);

        SDL_SetRenderDrawColor(renderer, colorClear.r, colorClear.g, colorClear.b, colorClear.a);
        SDL_RenderClear(renderer);
        std::fill(zBuffer.begin(), zBuffer.end(), std::numeric_limits<double>::max());

        std::vector<std::thread> threadS;

        for (const BuildingModel& model : models) {
            threadS.emplace_back(render, *model.v, model.uniform, model.i);
        }
        for (std::thread& thread : threadS) {
            thread.join();
        }

        SDL_RenderPresent(renderer);
        frameTime = SDL_GetTicks() - startingFrame;
        frameCounter++;
        if (frameTime >= 1000) {
            fps = frameCounter;
            frameCounter = 0;
            startingFrame = SDL_GetTicks(); // Reinicia el tiempo de inicio para el siguiente segundo
        }
        std::string fpsMessage = "Space Travel | FPS: " + std::to_string(fps);
        SDL_SetWindowTitle(window, fpsMessage.c_str());
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
