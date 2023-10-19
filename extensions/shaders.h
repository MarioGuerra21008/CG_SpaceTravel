#include "glm/glm.hpp"
#include "uniform.h"
#include "fragment.h"
#include "color.h"
#include "vertexArray.h"
#include "FastNoiseLite.h"
#pragma once

void printMatrix(const glm::mat4& myMatrix) {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            std::cout << myMatrix[i][j] << " ";
        }
        std::cout << std::endl;
    }
}

void printVec4(const glm::vec4& vector) {
    std::cout << "(" << vector.x << ", " << vector.y << ", " << vector.z << ", " << vector.w << ")";
}

void printVec3(const glm::vec3& vector) {
    std::cout << "(" << vector.x << ", " << vector.y << ", " << vector.z << ")";
}

Vertex vertexShader(const Vertex& vertex, const Uniform& uniform) {
    glm::vec4 transformedVertex = uniform.viewport * uniform.projection * uniform.view * uniform.model * glm::vec4(vertex.position, 1.0f);
    glm::vec3 vertexRedux;
    vertexRedux.x = transformedVertex.x / transformedVertex.w;
    vertexRedux.y = transformedVertex.y / transformedVertex.w;
    vertexRedux.z = transformedVertex.z / transformedVertex.w;
    Color fragmentColor(255, 0, 0, 255);
    glm::vec3 normal = glm::normalize(glm::mat3(uniform.model) * vertex.normal);
    Fragment fragment;
    fragment.position = glm::ivec2(transformedVertex.x, transformedVertex.y);
    fragment.color = fragmentColor;
    return Vertex {vertexRedux, normal, vertex.tex, vertex.position};
}

float nextTime = 0.5f;

Color fragmentShader(Fragment& fragment) {
    return fragment.color;
}

Color fragmentShaderSun(Fragment& fragment) {
    // Obtiene las coordenadas del fragmento en el espacio 2D
    glm::vec2 fragmentCoords(fragment.original.x, fragment.original.y);

    // Crear un objeto FastNoiseLite para generar ruido
    FastNoiseLite noise;

    // Configuración del ruido
    noise.SetNoiseType(FastNoiseLite::NoiseType_Cellular); // Tipo de ruido celular
    noise.SetSeed(1500); // Semilla para la generación de ruido (puedes cambiarla)
    noise.SetFrequency(0.005f); // Frecuencia del ruido (ajusta según tus preferencias)

    // Configuración de ruido fractal para variaciones
    noise.SetFractalType(FastNoiseLite::FractalType_PingPong); // Tipo de ruido fractal
    noise.SetFractalOctaves(2); // Número de octavas
    noise.SetFractalLacunarity(10 + nextTime); // Lacunarity (variación en la frecuencia)
    noise.SetFractalGain(1.0f); // Ganancia
    noise.SetFractalWeightedStrength(0.80f); // Fuerza ponderada
    noise.SetFractalPingPongStrength(10); // Fuerza de ping pong

    // Configuración de ruido celular
    noise.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Euclidean); // Función de distancia celular
    noise.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance2Add); // Tipo de retorno celular
    noise.SetCellularJitter(20); // Jitter (variación en las celdas)

    // Definir el rango de colores desde amarillo hasta rojo
    Color yellowColor(255, 255, 0, 255);
    Color redColor(255, 75, 0, 255);
    Color flareColor(255, 0, 0, 255); // Color de las llamaradas (ajusta según tus preferencias)

    // Parámetros para la rotación del sol
    float ox = 3000.0f; // Desplazamiento en X
    float oy = 3000.0f; // Desplazamiento en Y
    float zoom = 5000.0f; // Factor de zoom (ajusta según tus preferencias)

    // Obtener el valor de ruido en función de la posición y el zoom
    float noiseValue = abs(noise.GetNoise((fragment.original.x + ox) * zoom, (fragment.original.y + oy) * zoom, fragment.original.z * zoom));

    // Aplicar un color amarillo si el valor de ruido es bajo, de lo contrario, aplicar rojo
    Color tmpColor = (noiseValue < 0.5f) ? redColor : yellowColor;

    // Agregar efecto de llamaradas de fuego
    float flareIntensity = 2.0f;
    float oxf = 6000.0f; // Desplazamiento en X
    float oyf = 6000.0f; // Desplazamiento en Y
    float zoomf = 8000.0f; // Factor de zoom (ajusta según tus preferencias)

    float noiseValuef = abs(noise.GetNoise((fragment.original.x + oxf) * zoomf, (fragment.original.y + oyf) * zoomf, fragment.original.z * zoomf));

    if (noiseValuef > 0.9f) {
      /*  tmpColor = flareColor; */
    }

    // Multiplicar el color por la coordenada Z para simular la perspectiva
    fragment.color = tmpColor * fragment.z;
    fragment.color = fragment.color + (flareColor * flareIntensity);

    // Incrementar la variable "nextTime" para animar el ruido (rotación)
    nextTime += 0.5f; // Puedes ajustar la velocidad de rotación

    return fragment.color;
}

Color fragmentShaderMercury(Fragment& fragment) {
    return fragment.color;
}

Color fragmentShaderVenus(Fragment& fragment) {
    return fragment.color;
}

Color fragmentShaderEarth(Fragment& fragment) {
    // Obtiene las coordenadas del fragmento en el espacio 2D
    glm::vec2 fragmentCoords(fragment.original.x, fragment.original.y);

    // Aplicar un ruido para simular las características de la superficie terrestre
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetSeed(12000);
    noise.SetFrequency(0.0002f);
    noise.SetFractalType(FastNoiseLite::FractalType_Ridged);
    noise.SetFractalOctaves(3);
    noise.SetFractalLacunarity(10.0f + nextTime);
    noise.SetFractalGain(0.2f);
    noise.SetFractalWeightedStrength(0.50f); // Fuerza ponderada
    noise.SetFractalPingPongStrength(10); // Fuerza de ping pong

    // Configuración de ruido celular
    noise.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Euclidean); // Función de distancia celular
    noise.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance2Add); // Tipo de retorno celular
    noise.SetCellularJitter(6); // Jitter (variación en las celdas)

    // Definir colores para el agua y la tierra
    Color waterColor(0, 0, 255, 255); // Color del agua
    Color landColor(0, 128, 0, 255);  // Color de la tierra
    Color cloudColor(233, 239, 240, 200);  // Color de las nubes
    Color polarColor(255, 255, 255, 255); //Color de los polos

    // Parámetros para la rotación del sol
    float ox = 3000.0f; // Desplazamiento en X
    float oy = 3000.0f; // Desplazamiento en Y
    float zoom = 5000.0f; // Factor de zoom (ajusta según tus preferencias)

    // Obtener el valor de ruido en función de la posición y el zoom
    float noiseValue = abs(noise.GetNoise((fragment.original.x + ox) * zoom, (fragment.original.y + oy) * zoom, fragment.original.z * zoom));

    // Seleccionar el color en función del valor de ruido y el umbral
    Color tmpColor = (noiseValue < 0.3f) ? waterColor : landColor;

    float oxc = 5000.0f; // Desplazamiento en X
    float oyc = 5000.0f; // Desplazamiento en Y
    float zoomc = 8000.0f; // Factor de zoom (ajusta según tus preferencias)

    float noiseValueC = abs(noise.GetNoise((fragment.original.x + oxc) * zoomc, (fragment.original.y + oyc) * zoomc, fragment.original.z * zoomc));

    if (noiseValueC > 0.5f) {
        tmpColor = cloudColor;
    }

    // Multiplicar el color por la coordenada Z para simular la perspectiva
    fragment.color = tmpColor * fragment.z;

    // Parámetros para la ubicación y el tamaño de la Gran Mancha Roja
    glm::vec2 northPoleCenter(0.0f, 0.7f);
    glm::vec2 southPoleCenter(0.0f, -0.7f);
    float poleRadius = 0.33f;

    // Calcular la distancia desde el centro para simular la Gran Mancha Roja
    float distanceToCenterN = glm::length(fragmentCoords - northPoleCenter);
    float distanceToCenterS = glm::length(fragmentCoords - southPoleCenter);

    // Verificar si el fragmento está dentro y asignar su color
    if (distanceToCenterN <= poleRadius) {
        fragment.color = polarColor;
    } else {
        fragment.color = fragment.color;
    }
    if (distanceToCenterS <= poleRadius) {
        fragment.color = polarColor;
    } else {
        fragment.color = fragment.color;
    }

    // Incrementar la variable "nextTime" para animar el ruido (rotación)
    nextTime += 0.5f; // Puedes ajustar la velocidad de rotación

    return fragment.color;
}

Color fragmentShaderMars(Fragment& fragment) {
    // Obtiene las coordenadas del fragmento en el espacio 2D
    glm::vec2 fragmentCoords(fragment.original.x, fragment.original.y);

    // Aplicar un ruido para simular las características de la superficie terrestre
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetSeed(2050);
    noise.SetFrequency(0.006f);
    noise.SetFractalType(FastNoiseLite::FractalType_Ridged);
    noise.SetFractalOctaves(2);
    noise.SetFractalLacunarity(4.0f + nextTime);
    noise.SetFractalGain(0.8f);
    noise.SetFractalWeightedStrength(0.80f); // Fuerza ponderada
    noise.SetFractalPingPongStrength(8); // Fuerza de ping pong

    // Configuración de ruido celular
    noise.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Euclidean); // Función de distancia celular
    noise.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance2Add); // Tipo de retorno celular
    noise.SetCellularJitter(6); // Jitter (variación en las celdas)

    // Definir colores
    Color marsColor1(184, 73, 46, 255); // Color de Neptuno principal
    Color marsColor2(138, 77, 62, 255);

    // Parámetros para la rotación del sol
    float ox = 3000.0f; // Desplazamiento en X
    float oy = 3000.0f; // Desplazamiento en Y
    float zoom = 5000.0f; // Factor de zoom (ajusta según tus preferencias)

    // Obtener el valor de ruido en función de la posición y el zoom
    float noiseValue = abs(noise.GetNoise((fragment.original.x + ox) * zoom, (fragment.original.y + oy) * zoom, fragment.original.z * zoom));

    // Seleccionar el color en función del valor de ruido y el umbral
    Color tmpColor = (noiseValue < 0.4f) ? marsColor1 : marsColor2;

    // Multiplicar el color por la coordenada Z para simular la perspectiva
    fragment.color = tmpColor * fragment.z;

    // Incrementar la variable "nextTime" para animar el ruido (rotación)
    nextTime += 0.5f; // Puedes ajustar la velocidad de rotación

    return fragment.color;
}

Color fragmentShaderJupiter(Fragment& fragment) {
    // Obtener las coordenadas del fragmento en el espacio 2D
    glm::vec2 fragmentCoords(fragment.original.x, fragment.original.y);

    // Parámetros para el ruido y la rotación
    float ox = 3000.0f; // Desplazamiento en X
    float oy = 3000.0f; // Desplazamiento en Y
    float zoom = 5000.0f; // Factor de zoom (ajusta según tus preferencias)

    // Obtener el valor de ruido en función de la posición y el zoom
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetSeed(1384);
    noise.SetFrequency(0.005f);
    noise.SetFractalType(FastNoiseLite::FractalType_Ridged);
    noise.SetFractalOctaves(3);
    noise.SetFractalLacunarity(5.0f + nextTime);
    noise.SetFractalGain(0.9f);
    noise.SetFractalWeightedStrength(0.90f); // Fuerza ponderada
    noise.SetFractalPingPongStrength(1); // Fuerza de ping pong

    // Configuración de ruido celular
    noise.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Euclidean); // Función de distancia celular
    noise.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance2Add); // Tipo de retorno celular
    noise.SetCellularJitter(10); // Jitter (variación en las celdas)

    // Definir colores para Júpiter y la Gran Mancha Roja
    Color jupiterColor(255, 164, 81, 255); // Color de Júpiter
    Color redSpotColor(240, 138, 65, 255);  // Color de la Gran Mancha Roja

    // Obtener el valor de ruido en función de la posición y el zoom
    float noiseValue = abs(noise.GetNoise((fragment.original.x + ox) * zoom, (fragment.original.y + oy) * zoom, fragment.original.z * zoom));

    // Mapear el valor de ruido al rango de colores para Júpiter
    float tmp = (noiseValue + 1.0f) / 2.0f; // Asegura que esté en el rango [0, 1]
    Color jupiterColorFinal = jupiterColor * tmp;

    // Parámetros para la ubicación y el tamaño de la Gran Mancha Roja
    glm::vec2 redSpotCenter(0.2f, -0.15f);
    float redSpotRadius = 0.025f;

    // Calcular la distancia desde el centro para simular la Gran Mancha Roja
    float distanceToCenter = glm::length(fragmentCoords - redSpotCenter);

    // Verificar si el fragmento está dentro de la Gran Mancha Roja y asignar su color
    if (distanceToCenter <= redSpotRadius) {
        fragment.color = redSpotColor;
    } else {
        fragment.color = jupiterColorFinal;
    }

    // Incrementar la variable "nextTime" para animar el ruido (rotación)
    nextTime += 0.2f; // Puedes ajustar la velocidad de rotación

    return fragment.color;
}

Color fragmentShaderSaturn(Fragment& fragment) {
    // Obtiene las coordenadas del fragmento en el espacio 2D
    glm::vec2 fragmentCoords(fragment.original.x, fragment.original.y);

    // Obtener el valor de ruido en función de la posición y el zoom
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetSeed(1000);
    noise.SetFrequency(0.005f);
    noise.SetFractalType(FastNoiseLite::FractalType_Ridged);
    noise.SetFractalOctaves(1);
    noise.SetFractalLacunarity(5.0f + nextTime);
    noise.SetFractalGain(0.5f);
    noise.SetFractalWeightedStrength(0.90f); // Fuerza ponderada
    noise.SetFractalPingPongStrength(2); // Fuerza de ping pong

    // Configuración de ruido celular
    noise.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Euclidean); // Función de distancia celular
    noise.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance2Add); // Tipo de retorno celular
    noise.SetCellularJitter(5); // Jitter (variación en las celdas)

    // Definir colores para el planeta Saturno y los anillos
    Color saturnColor1(206, 184, 184, 255); // Color del planeta Saturno
    Color saturnColor2(234,214,184, 255); // Color del planeta Saturno
    Color ringColor(255, 255, 255, 255);   // Color de los anillos de Saturno

    // Parámetros para el ruido y la rotación
    float ox = 3000.0f; // Desplazamiento en X
    float oy = 3000.0f; // Desplazamiento en Y
    float zoom = 5000.0f; // Factor de zoom (ajusta según tus preferencias)

    // Obtener el valor de ruido en función de la posición y el zoom
    float noiseValue = abs(noise.GetNoise((fragment.original.x + ox) * zoom, (fragment.original.y + oy) * zoom, fragment.original.z * zoom));

    // Mapear el valor de ruido al rango de colores
    float tmp = (noiseValue + 1.0f) / 2.0f; // Asegura que esté en el rango [0, 1]

    // Mezcla los colores del vórtice basados en el valor de ruido
    Color saturnColor = saturnColor2 * (1.0f - tmp) + saturnColor1 * tmp;

    fragment.color = saturnColor;

    // Parámetros para los anillos
    float ringInnerRadius = 0.6f;
    float ringOuterRadius = 0.8f;

    // Incrementar la variable "nextTime" para animar el ruido (rotación)
    nextTime += 0.5f; // Puedes ajustar la velocidad de rotación

    return fragment.color;
}

Color fragmentShaderUranus(Fragment& fragment) {
    // Obtiene las coordenadas del fragmento en el espacio 2D
    glm::vec2 fragmentCoords(fragment.original.x, fragment.original.y);

    // Aplicar un ruido para simular las características de la superficie terrestre
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetSeed(2000);
    noise.SetFrequency(0.009f);
    noise.SetFractalType(FastNoiseLite::FractalType_Ridged);
    noise.SetFractalOctaves(2);
    noise.SetFractalLacunarity(2.0f + nextTime);
    noise.SetFractalGain(0.5f);
    noise.SetFractalWeightedStrength(0.80f); // Fuerza ponderada
    noise.SetFractalPingPongStrength(4); // Fuerza de ping pong

    // Configuración de ruido celular
    noise.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Euclidean); // Función de distancia celular
    noise.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance2Add); // Tipo de retorno celular
    noise.SetCellularJitter(2); // Jitter (variación en las celdas)

    // Definir colores
    Color uranusColor1(218, 242, 242, 255); // Color de Neptuno principal
    Color uranusColor2(209,231,231, 255);

    // Parámetros para la rotación del sol
    float ox = 1000.0f; // Desplazamiento en X
    float oy = 1000.0f; // Desplazamiento en Y
    float zoom = 3000.0f; // Factor de zoom (ajusta según tus preferencias)

    // Obtener el valor de ruido en función de la posición y el zoom
    float noiseValue = abs(noise.GetNoise((fragment.original.x + ox) * zoom, (fragment.original.y + oy) * zoom, fragment.original.z * zoom));

    // Seleccionar el color en función del valor de ruido y el umbral
    Color tmpColor = (noiseValue < 0.9f) ? uranusColor1 : uranusColor2;

    // Multiplicar el color por la coordenada Z para simular la perspectiva
    fragment.color = tmpColor * fragment.z;

    // Incrementar la variable "nextTime" para animar el ruido (rotación)
    nextTime += 0.5f; // Puedes ajustar la velocidad de rotación

    return fragment.color;
}

Color fragmentShaderNeptune(Fragment& fragment) {
    // Obtiene las coordenadas del fragmento en el espacio 2D
    glm::vec2 fragmentCoords(fragment.original.x, fragment.original.y);

    // Aplicar un ruido para simular las características de la superficie terrestre
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetSeed(23838);
    noise.SetFrequency(0.0023f);
    noise.SetFractalType(FastNoiseLite::FractalType_Ridged);
    noise.SetFractalOctaves(1);
    noise.SetFractalLacunarity(2.0f + nextTime);
    noise.SetFractalGain(0.5f);
    noise.SetFractalWeightedStrength(0.80f); // Fuerza ponderada
    noise.SetFractalPingPongStrength(4); // Fuerza de ping pong

    // Configuración de ruido celular
    noise.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Euclidean); // Función de distancia celular
    noise.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance2Add); // Tipo de retorno celular
    noise.SetCellularJitter(2); // Jitter (variación en las celdas)

    // Definir colores
    Color neptuneColor1(63,84,186, 255); // Color de Neptuno principal
    Color neptuneColor2(91,93,223, 255);
    Color cloudColor(255, 255, 255, 255);  // Color de las nubes

    // Parámetros para la rotación del sol
    float ox = 1000.0f; // Desplazamiento en X
    float oy = 1000.0f; // Desplazamiento en Y
    float zoom = 3000.0f; // Factor de zoom (ajusta según tus preferencias)

    // Obtener el valor de ruido en función de la posición y el zoom
    float noiseValue = abs(noise.GetNoise((fragment.original.x + ox) * zoom, (fragment.original.y + oy) * zoom, fragment.original.z * zoom));

    // Seleccionar el color en función del valor de ruido y el umbral
    Color tmpColor = (noiseValue < 0.9f) ? neptuneColor1 : neptuneColor2;

    float oxc = 4000.0f; // Desplazamiento en X
    float oyc = 4000.0f; // Desplazamiento en Y
    float zoomc = 6000.0f; // Factor de zoom (ajusta según tus preferencias)

    float noiseValueC = abs(noise.GetNoise((fragment.original.x + oxc) * zoomc, (fragment.original.y + oyc) * zoomc, fragment.original.z * zoomc));

    if (noiseValueC < 0.008f) {
        tmpColor = cloudColor;
    }

    // Multiplicar el color por la coordenada Z para simular la perspectiva
    fragment.color = tmpColor * fragment.z;

    // Incrementar la variable "nextTime" para animar el ruido (rotación)
    nextTime += 0.5f; // Puedes ajustar la velocidad de rotación

    return fragment.color;
}

Color fragmentShaderMoon(Fragment& fragment) {
    return fragment.color;
}
