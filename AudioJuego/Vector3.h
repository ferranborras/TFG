#ifndef VECTOR3_H
#define VECTOR3_H

#include <Arduino.h>
#include <math.h>

struct Vector3 {
    float x;
    float y;
    float z;

    Vector3() : x(0), y(0), z(0) {}
    Vector3(float newX, float newY, float newZ) : x(newX), y(newY), z(newZ) {}

    float magnitude() const {
        return sqrt(x * x + y * y + z * z);
    }

    Vector3 normalized() const {
        float mag = magnitude();
        if (mag == 0) return Vector3(0, 0, 0);
        return Vector3(x / mag, y / mag, z / mag);
    }

    float dot(const Vector3& other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    float distance(const Vector3& other) const {
        float dx = x - other.x;
        float dy = y - other.y;
        float dz = z - other.z;
        return sqrt(dx * dx + dy * dy + dz * dz);
    }

    Vector3 vectorTo(const Vector3& other) const {
        return Vector3(other.x - x, other.y - y, other.z - z);
    }

    Vector3 add(const Vector3& other) const {
        return Vector3(x + other.x, y + other.y, z + other.z);
    }

    Vector3 multiply(float factor) const {
        return Vector3(x*factor, y*factor, z*factor);
    }

    float azimuth() {
        float azimuth = atan2(x, y) * 180.0 / PI;   // Azimuth
        azimuth = fmod(azimuth + 360.0, 360.0);  // Convertir azimuth a un rango [0, 360)
        return azimuth;
    }

    float elevation() {
        float elevation = atan2(z, sqrt(x * x + y * y)) * 180.0 / PI;  // Elevación
        elevation = constrain(elevation, -90.0, 90.0);  // Constrains elevación
        return elevation;
    }

    void print() {
        Serial.printf("\nLoc: (%f, %f, %f)\n", x, y, z);
        //Serial.printf("Az: %f | El: %f\n", azimuth(), elevation());
        //Serial.printf("Dst: %f\n\n", distance(Vector3(0, 0, 0)));
    }
};

#endif