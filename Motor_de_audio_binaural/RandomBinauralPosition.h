#ifndef RANDOM_BINAURAL_POSITION_H
#define RANDOM_BINAURAL_POSITION_H

#include <random>
#include <cmath>
#include "Vector3.h"  // Asume que tienes tu clase Vector3

class RandomBinauralPosition {
private:
    Vector3 currentPos;
    Vector3 targetPos;
    float speed = 0.5f;  // Velocidad de transición (ajustable)
    float minRadius;
    float maxRadius;

    // Generador de números aleatorios
    std::mt19937 rng;
    std::uniform_real_distribution<float> distRadius;
    std::uniform_real_distribution<float> distAngle;

public:
    RandomBinauralPosition(float minR, float maxR) 
        : minRadius(minR), maxRadius(maxR),
          rng(std::random_device{}()),
          distRadius(minR, maxR),
          distAngle(0.0f, 2.0f * M_PI) {
        
        // Posición inicial aleatoria
        currentPos = generateRandomPosition();
        targetPos = generateRandomPosition();
    }

    Vector3 update(float deltaTime) {
        // Mover suavemente hacia el target
        Vector3 direction = currentPos.vectorTo(targetPos).normalized();
        float distance = currentPos.vectorTo(targetPos).magnitude();

        if (distance < 0.1f) {
            // Elegir nuevo target al llegar cerca
            targetPos = generateRandomPosition();
        } else {
            // Interpolación lineal
            direction = direction.multiply(speed).multiply(deltaTime);
            currentPos = currentPos.add(direction);
        }

        return currentPos;
    }

private:
    Vector3 generateRandomPosition() {
        // Coordenadas esféricas aleatorias
        float radius = distRadius(rng);
        float theta = distAngle(rng);  // Ángulo en el plano XY
        float phi = std::acos(2.0f * distAngle(rng) / (2.0f * M_PI) - 1.0f);  // Ángulo de elevación

        // Convertir a cartesianas
        float x = radius * std::sin(phi) * std::cos(theta);
        float y = radius * std::sin(phi) * std::sin(theta);
        float z = radius * std::cos(phi);

        return Vector3(x, y, z);
    }
};

#endif