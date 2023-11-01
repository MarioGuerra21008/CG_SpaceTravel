#include <SDL.h>
#include <mutex>
#include <vector>
#include <thread>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "extensions/color.h"
#include "extensions/loadOBJFile.h"
#include "extensions/shaders.h"
#include "extensions/uniform.h"
#include "extensions/vertexArray.h"

const int WINDOW_WIDTH = 720;
const int WINDOW_HEIGHT = 480;
float pi = 3.14f / 3.0f;
std::mutex mutex;

Uint32 startingFrame;
Uint32 frameTime;
int frameCounter = 0;
int fps = 0;

SDL_Renderer* renderer;
std::array<double, WINDOW_WIDTH * WINDOW_HEIGHT> zBuffer;

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

Color clearColor = {0, 0, 0, 255};

glm::vec3 light = glm::vec3(0, 0, 200.0f);

Color interpolateColor(const glm::vec3& barycentricCoord, const Color& colorA, const Color& colorB, const Color& colorC) {
    float u = barycentricCoord.x;
    float v = barycentricCoord.y;
    float w = barycentricCoord.z;

    uint8_t r = static_cast<uint8_t>(u * colorA.r + v * colorB.r + w * colorC.r);
    uint8_t g = static_cast<uint8_t>(u * colorA.g + v * colorB.g + w * colorC.g);
    uint8_t b = static_cast<uint8_t>(u * colorA.b + v * colorB.b + w * colorC.b);
    uint8_t a = static_cast<uint8_t>(u * colorA.a + v * colorB.a + w * colorC.a);

    return Color(r, g, b, a);
}

bool isBarycentricCoord(const glm::vec3& barycentricCoord) {
    return barycentricCoord.x >= 0 && barycentricCoord.y >= 0 && barycentricCoord.z >= 0 &&
           barycentricCoord.x <= 1 && barycentricCoord.y <= 1 && barycentricCoord.z <= 1 &&
           glm::abs(1 - (barycentricCoord.x + barycentricCoord.y + barycentricCoord.z)) < 0.005f;
}

glm::vec3 calculateBarycentricCoord(const glm::vec2& A, const glm::vec2& B, const glm::vec2& C, const glm::vec2& P) {
    float denominator = (B.y - C.y) * (A.x - C.x) + (C.x - B.x) * (A.y - C.y);
    float u = ((B.y - C.y) * (P.x - C.x) + (C.x - B.x) * (P.y - C.y)) / denominator;
    float v = ((C.y - A.y) * (P.x - C.x) + (A.x - C.x) * (P.y - C.y)) / denominator;
    float w = 1 - u - v;
    return glm::vec3(u, v, w);
}

glm::mat4 createModelSpace() {
    glm::mat4 translation = glm::translate(glm::mat4(1), glm::vec3(0.0f, 0.0f, -10.0f));
    glm::mat4 scale = glm::scale(glm::mat4(1), glm::vec3(40.0f, 40.0f, 10.0f));
    glm::mat4 rotation = glm::mat4(1);
    return translation * scale * rotation;
}

glm::mat4 createModelPlanet(glm::vec3 translationM, glm::vec3 scaleM, glm::vec3 rotationM, float radianSpeed)  {
    glm::mat4 translation = glm::translate(glm::mat4(1), translationM);
    glm::mat4 scale = glm::scale(glm::mat4(1), scaleM);
    glm::mat4 rotation = glm::rotate(glm::mat4(1), glm::radians((pi++)*radianSpeed), rotationM);
    return translation * scale * rotation;
}

glm::mat4 createModelSpaceship(glm::vec3 cameraPosition, glm::vec3 targetPosition,glm::vec3 upVector, float rotX, float rotY) {
    glm::mat4 translation = glm::translate(glm::mat4(1), (targetPosition - cameraPosition) / 3.0f + cameraPosition - upVector * 0.15f);
    glm::mat4 scale = glm::scale(glm::mat4(1), glm::vec3(0.05f, 0.05f, 0.05f));
    glm::mat4 rotationX = glm::rotate(glm::mat4(1), glm::radians(-rotX), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 rotationY = glm::rotate(glm::mat4(1), glm::radians(-rotY), glm::vec3(0.0f, 0.0f, 1.0f));
    return translation * scale * rotationX * rotationY;
}

glm::mat4 createProjectionMatrix() {
    float fovInDegrees = 45.0f;
    float aspectRatio = WINDOW_WIDTH / WINDOW_HEIGHT;
    float nearClip = 0.1f;
    float farClip = 100.0f;
    return glm::perspective(glm::radians(fovInDegrees), aspectRatio, nearClip, farClip);
}

glm::mat4 createViewportMatrix() {
    glm::mat4 viewport = glm::mat4(1.0f);
    viewport = glm::scale(viewport, glm::vec3(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f, 0.5f));
    viewport = glm::translate(viewport, glm::vec3(1.0f, 1.0f, 0.5f));
    return viewport;
}

glm::vec3 calculatePositionInCircle(float angleR, float radius){
    float posX = glm::cos(angleR) * radius;
    float posZ = glm::sin(angleR) * radius;
    return glm::vec3(posX, 0.0f, posZ);
}

Uniform uniform;
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

        int minX = static_cast<int>(std::min({A.x, B.x, C.x}));
        int minY = static_cast<int>(std::min({A.y, B.y, C.y}));
        int maxX = static_cast<int>(std::max({A.x, B.x, C.x}));
        int maxY = static_cast<int>(std::max({A.y, B.y, C.y}));

        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                if (y>0 && y<WINDOW_HEIGHT && x>0 && x<WINDOW_WIDTH) {
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

                            switch (planetIdentifier) {
                                case SPACE:
                                    fragmentShaderf = fragmentShader(fragment);
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
                                case SHIP:
                                    fragmentShaderf = fragmentShaderSpaceship(fragment);
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

int main(int argc, char* argv[]) {

    SDL_Init(SDL_INIT_EVERYTHING);
    startingFrame = SDL_GetTicks();
    SDL_Window* window = SDL_CreateWindow("Space Travel", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    glm::vec3 cameraPosition(0.0f, 0.0f, 17.0f);
    glm::vec3 targetPosition(0.0f, 0.0f, 0.0f);
    glm::vec3 upVector(0.0f, 1.0f, 0.0f);

    std::vector<BuildingModel> models;
    int renderWidth, renderHeight;
    SDL_GetRendererOutputSize(renderer, &renderWidth, &renderHeight);

    std::vector<glm::vec3> planetVertices;
    std::vector<glm::vec3> planetNormal;
    std::vector<Face> planetFaces;

    bool success = loadOBJ("../models/sphere.obj", planetVertices, planetNormal, planetFaces);
    if (!success) {
        return 1;
    }
    std::vector<Vertex> vertexArrayPlanet = setupVertexArray(planetVertices, planetNormal, planetFaces);

    std::vector<glm::vec3> spaceshipVertices;
    std::vector<glm::vec3> spaceshipNormal;
    std::vector<Face> spaceshipFaces;

    bool success2 = loadOBJ("../models/Lab3.obj", spaceshipVertices, spaceshipNormal, spaceshipFaces);
    if (!success2) {
        return 1;
    }
    std::vector<Vertex> vertexArrayShip = setupVertexArray(spaceshipVertices, spaceshipNormal, spaceshipFaces);

    float forwardBackwardMovementSpeed = 0.1f;
    float leftRightMovementSpeed = 0.06f;
    float rotationX = 0.0f;
    float rotationY = 0.0f;
    float sunRotation = 0.0f;
    float earthRotation = 0.0f;
    float marsRotation = 0.0f;
    float jupiterRotation = 0.0f;
    float saturnRotation = 0.0f;
    float uranusRotation = 0.0f;
    float neptuneRotation = 0.0f;

    glm::vec3 newCameraPosition;
    glm::vec3 newShipCameraPosition;

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            else if (event.type = SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_w:
                        cameraPosition += forwardBackwardMovementSpeed * glm::normalize(targetPosition - cameraPosition);
                        targetPosition += forwardBackwardMovementSpeed * (targetPosition - cameraPosition);
                        break;
                    case SDLK_a:
                        cameraPosition -= leftRightMovementSpeed * glm::normalize(glm::cross((targetPosition - cameraPosition), upVector)) * 2.5f;
                        targetPosition -= leftRightMovementSpeed * glm::normalize(glm::cross((targetPosition - cameraPosition), upVector)) * 2.5f;
                        break;
                    case SDLK_s:
                        cameraPosition -= forwardBackwardMovementSpeed * glm::normalize(targetPosition - cameraPosition);
                        targetPosition -= forwardBackwardMovementSpeed * (targetPosition - cameraPosition);
                        break;
                    case SDLK_d:
                        cameraPosition += leftRightMovementSpeed * glm::normalize(glm::cross((targetPosition - cameraPosition), upVector)) * 2.5f;
                        targetPosition += leftRightMovementSpeed * glm::normalize(glm::cross((targetPosition - cameraPosition), upVector)) * 2.5f;
                        break;
                    case SDLK_i:
                        rotationY += 1.0f;
                        break;
                    case SDLK_j:
                        rotationX -= 1.0f;
                        break;
                    case SDLK_k:
                        rotationY -= 1.0f;
                        break;
                    case SDLK_l:
                        rotationX += 1.0f;
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
        targetPosition = glm::vec3(5.0f * sin(glm::radians(rotationX)) * cos(glm::radians(rotationY)), 5.0f * sin(glm::radians(rotationY)), -5.0f * cos(glm::radians(rotationX)) * cos(glm::radians(rotationY))) + cameraPosition;

        glm::vec3 translateEarth = calculatePositionInCircle(earthRotation, 2.0f);
        glm::vec3 translateMars = calculatePositionInCircle(marsRotation, 2.5f);
        glm::vec3 translateJupiter = calculatePositionInCircle(jupiterRotation, 3.5f);
        glm::vec3 translateSaturn = calculatePositionInCircle(saturnRotation, 4.5f);
        glm::vec3 translateUranus = calculatePositionInCircle(uranusRotation, 5.5f);
        glm::vec3 translateNeptune = calculatePositionInCircle(neptuneRotation, 6.25f);

        uniform.model = createModelSpace();
        uniform.view = glm::lookAt(cameraPosition, targetPosition, upVector);
        uniform.projection = createProjectionMatrix();
        uniform.viewport = createViewportMatrix();

        model1.uniform = uniform;
        model1.v = &vertexArrayPlanet;
        model1.i = SPACE;

        uniform2.model = createModelPlanet(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.5f, 1.5f, 1.5f), glm::vec3(0.0f, 1.0f, 0.0f), 0.1f);
        uniform2.view =  glm::lookAt(cameraPosition, targetPosition, upVector);
        uniform2.projection = createProjectionMatrix();
        uniform2.viewport = createViewportMatrix();

        model2.uniform = uniform2;
        model2.v = &vertexArrayPlanet;
        model2.i = SUN;

        uniform3.model = createModelPlanet(translateEarth, glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(0.0f, 1.0f, 0.0f), 0.35f);
        uniform3.view = glm::lookAt(cameraPosition, targetPosition, upVector);
        uniform3.projection = createProjectionMatrix();
        uniform3.viewport = createViewportMatrix();

        model3.uniform = uniform3;
        model3.v = &vertexArrayPlanet;
        model3.i = EARTH;

        uniform4.model = createModelPlanet(translateMars, glm::vec3(0.45f, 0.45f, 0.45f), glm::vec3(0.0f, 1.0f, 0.0f), 0.3f);
        uniform4.view = glm::lookAt(cameraPosition, targetPosition, upVector);
        uniform4.projection = createProjectionMatrix();
        uniform4.viewport = createViewportMatrix();

        model4.uniform = uniform4;
        model4.v = &vertexArrayPlanet;
        model4.i = MARS;

        uniform5.model = createModelPlanet(translateJupiter, glm::vec3(0.8f, 0.8f, 0.8f), glm::vec3(0.0f, 1.0f, 0.0f), 0.15f);
        uniform5.view = glm::lookAt(cameraPosition, targetPosition, upVector);
        uniform5.projection = createProjectionMatrix();
        uniform5.viewport = createViewportMatrix();

        model5.uniform = uniform5;
        model5.v = &vertexArrayPlanet;
        model5.i = JUPITER;

        uniform6.model = createModelPlanet(translateSaturn, glm::vec3(0.65f, 0.65f, 0.65f), glm::vec3(0.0f, 1.0f, 0.0f), 0.2f);
        uniform6.view = glm::lookAt(cameraPosition, targetPosition, upVector);
        uniform6.projection = createProjectionMatrix();
        uniform6.viewport = createViewportMatrix();

        model6.uniform = uniform6;
        model6.v = &vertexArrayPlanet;
        model6.i = SATURN;

        uniform7.model = createModelPlanet(translateUranus, glm::vec3(0.6f, 0.6f, 0.6f), glm::vec3(0.0f, 1.0f, 0.0f), 0.2f);
        uniform7.view = glm::lookAt(cameraPosition, targetPosition, upVector);
        uniform7.projection = createProjectionMatrix();
        uniform7.viewport = createViewportMatrix();

        model7.uniform = uniform7;
        model7.v = &vertexArrayPlanet;
        model7.i = URANUS;

        uniform8.model = createModelPlanet(translateNeptune, glm::vec3(0.7f, 0.7f, 0.7f), glm::vec3(0.0f, 1.0f, 0.0f), 0.2f);
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

        models.push_back(model1);
        models.push_back(model2);
        models.push_back(model3);
        models.push_back(model4);
        models.push_back(model5);
        models.push_back(model6);
        models.push_back(model7);
        models.push_back(model8);
        models.push_back(model9);

        SDL_SetRenderDrawColor(renderer, clearColor.r, clearColor.g, clearColor.b, clearColor.a);
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
            startingFrame = SDL_GetTicks();
        }
        std::string fpsText = "Space Travel | FPS: " + std::to_string(fps);
        SDL_SetWindowTitle(window, fpsText.c_str());
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}