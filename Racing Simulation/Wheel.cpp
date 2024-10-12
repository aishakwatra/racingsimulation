#include "Wheel.h"

Wheel::Wheel(const glm::vec3& offsetPos) :
    offset(offsetPos),
    rotation(0.0f),
    steeringAngle(0.0f),
    maxSteeringAngle(45.0f),
    modelMatrix(glm::mat4(1.0f)) {}

void Wheel::updateModelMatrix(const glm::mat4& carModelMatrix, bool isSteeringWheel) {
    modelMatrix = glm::mat4(1.0f);
    modelMatrix = carModelMatrix;
    modelMatrix = glm::translate(modelMatrix, offset);

    if (isSteeringWheel) {
        modelMatrix = glm::rotate(modelMatrix, glm::radians(steeringAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    }

    modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation), glm::vec3(1.0f, 0.0f, 0.0f));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(1.0f, 1.0f, 1.0f));
}

glm::mat4 Wheel::getModelMatrix() const {
    return modelMatrix;
}

// Optional: Function to set the rotation of the wheel
void Wheel::setRotation(float newRotation) {
    rotation = newRotation;
}

// Optional: Function to set the steering angle of the wheel
void Wheel::setSteeringAngle(float newSteeringAngle) {
    steeringAngle = glm::clamp(newSteeringAngle, -maxSteeringAngle, maxSteeringAngle);
}
