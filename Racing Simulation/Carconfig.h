#pragma once

#include <glm/glm.hpp>

struct CarConfig {
    glm::vec3 position = glm::vec3(0.0f, 1.5f, 0.0f);
    glm::vec3 bodyOffset = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 bodyScale = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 wheelScale = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 direction = glm::vec3(0.0f, 0.0f, 1.0f);
    float rotation = 0.0f;
    float speed = 0.0f;
    float maxSpeed = 80.0f;
    float acceleration = 11.0f;
    float brakingForce = 20.0f;
    glm::vec3 frontLeftWheelOffset = glm::vec3(-0.65f, -0.6f, 0.85f);
    glm::vec3 frontRightWheelOffset = glm::vec3(0.65f, -0.6f, 0.85f);
    glm::vec3 backLeftWheelOffset = glm::vec3(-0.65f, -0.6f, -0.85f);
    glm::vec3 backRightWheelOffset = glm::vec3(0.65f, -0.6f, -0.85f);

    CarConfig() {}
};