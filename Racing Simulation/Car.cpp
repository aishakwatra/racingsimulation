#include "Car.h"
#include <iostream>

Car::Car(const CarConfig& config)
    : position(config.position), direction(config.direction), rotation(config.rotation),
    speed(config.speed), maxSpeed(config.maxSpeed), acceleration(config.acceleration),
    brakingForce(config.brakingForce), frontLeftWheel(config.frontLeftWheelOffset),
    frontRightWheel(config.frontRightWheelOffset), backLeftWheel(config.backLeftWheelOffset),
    backRightWheel(config.backRightWheelOffset),
    modelMatrix(glm::mat4(1.0f)) {}

void Car::applyConfig(const CarConfig& config) {
    position = config.position;
    direction = config.direction;
    rotation = config.rotation;
    speed = config.speed;
    maxSpeed = config.maxSpeed;
    acceleration = config.acceleration;
    brakingForce = config.brakingForce;
    frontLeftWheel.setOffset(config.frontLeftWheelOffset);
    frontRightWheel.setOffset(config.frontRightWheelOffset);
    backLeftWheel.setOffset(config.backLeftWheelOffset);
    backRightWheel.setOffset(config.backRightWheelOffset);
}
void Car::setCollisionGrid(const std::vector<std::vector<std::vector<Triangle>>>& gridCells, float gridSize, int gridWidth, int gridHeight) {
    collisionChecker.setGrid(gridCells, gridSize, gridWidth, gridHeight);
}

void Car::updateModelMatrix() {
    //instantiate rays
    glm::vec3 frontLeftOffset = glm::vec3(-0.65f, 1.0f, 0.85f);
    glm::vec3 frontRightOffset = glm::vec3(0.65f, 1.0f, 0.85f);
    glm::vec3 backLeftOffset = glm::vec3(-0.65f, 1.0f, -0.85f);
    glm::vec3 backRightOffset = glm::vec3(0.65f, 1.0f, -0.85f);

    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::vec3 frontLeftRayOrigin = position + glm::vec3(rotationMatrix * glm::vec4(frontLeftOffset, 1.0f));
    glm::vec3 frontRightRayOrigin = position + glm::vec3(rotationMatrix * glm::vec4(frontRightOffset, 1.0f));
    glm::vec3 backLeftRayOrigin = position + glm::vec3(rotationMatrix * glm::vec4(backLeftOffset, 1.0f));
    glm::vec3 backRightRayOrigin = position + glm::vec3(rotationMatrix * glm::vec4(backRightOffset, 1.0f));

    glm::vec3 rayDirection = glm::vec3(0.0f, -1.0f, 0.0f);

    //find intersections
    glm::vec3 frontLeftIntersection, frontRightIntersection, backLeftIntersection, backRightIntersection;
    collisionChecker.checkTrackIntersectionWithGrid(frontLeftRayOrigin, rayDirection, frontLeftIntersection);
    collisionChecker.checkTrackIntersectionWithGrid(frontRightRayOrigin, rayDirection, frontRightIntersection);
    collisionChecker.checkTrackIntersectionWithGrid(backLeftRayOrigin, rayDirection, backLeftIntersection);
    collisionChecker.checkTrackIntersectionWithGrid(backRightRayOrigin, rayDirection, backRightIntersection);

    //update car orientation
    glm::vec3 midFront = (frontLeftIntersection + frontRightIntersection) / 2.0f;
    glm::vec3 midBack = (backLeftIntersection + backRightIntersection) / 2.0f;

    float rollHeightDifference = ((frontRightIntersection.y + backRightIntersection.y) / 2.0f) - ((frontLeftIntersection.y + backLeftIntersection.y) / 2.0f);
    float pitchHeightDifference = ((frontLeftIntersection.y + frontRightIntersection.y) / 2.0f) - ((backLeftIntersection.y + backRightIntersection.y) / 2.0f);

    // compute pitch and roll
    float pitchAngle = glm::atan(-pitchHeightDifference / glm::length(midFront - midBack));
    float rollAngle = glm::atan(rollHeightDifference / glm::length(frontRightIntersection - frontLeftIntersection));

    // update car's y-position
    position.y = (frontLeftIntersection.y + frontRightIntersection.y + backLeftIntersection.y + backRightIntersection.y) / 4.0f + 1.5f;

    // Update the model matrix
    modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, position);
    modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.0f));  // Yaw (left and right)
    modelMatrix = glm::rotate(modelMatrix, pitchAngle, glm::vec3(1.0f, 0.0f, 0.0f));  // Pitch (up/down)
    modelMatrix = glm::rotate(modelMatrix, rollAngle, glm::vec3(0.0f, 0.0f, 1.0f));  // Roll (side-to-side)

    // Update wheels' model matrices
    frontLeftWheel.updateModelMatrix(modelMatrix, true);
    frontRightWheel.updateModelMatrix(modelMatrix, true);
    backLeftWheel.updateModelMatrix(modelMatrix, false);
    backRightWheel.updateModelMatrix(modelMatrix, false);

}

glm::mat4 Car::getModelMatrix() const {
    return modelMatrix;
}

// Accessors for wheel matrices
glm::mat4 Car::getFrontLeftWheelModelMatrix() const {
    return frontLeftWheel.getModelMatrix();
}

glm::mat4 Car::getFrontRightWheelModelMatrix() const {
    return frontRightWheel.getModelMatrix();
}

glm::mat4 Car::getBackLeftWheelModelMatrix() const {
    return backLeftWheel.getModelMatrix();
}

glm::mat4 Car::getBackRightWheelModelMatrix() const {
    return backRightWheel.getModelMatrix();
}


// Getters for position, direction, speed, and rotation
glm::vec3 Car::getPosition() const {
    return position;
}

glm::vec3 Car::getDirection() const {
    return direction;
}

float Car::getSpeed() const {
    return speed;
}

float Car::getRotation() const {
    return rotation;
}

float Car::getMaxSpeed() const {
    return maxSpeed;
}

void Car::accelerate(float deltaTime) {
    speed += acceleration * deltaTime;
    if (speed > maxSpeed) {
        speed = maxSpeed;
    }
}

void Car::brake(float deltaTime) {
    speed -= brakingForce * deltaTime;
    if (speed < -maxSpeed / 2.0f) {
        speed = -maxSpeed / 2.0f; // Limit reverse speed to half max speed
    }
}

void Car::slowDown(float deltaTime) {
    if (speed > 0) {
        speed -= acceleration * deltaTime;
        if (speed < 0) speed = 0; // Stop the car when speed reaches 0
    }
    else if (speed < 0) {
        speed += acceleration * deltaTime;
        if (speed > 0) speed = 0; // Stop the car when reverse speed reaches 0
    }
}

void Car::steerLeft(float deltaTime) {
    frontLeftWheel.steeringAngle += 120.0f * deltaTime;
    frontRightWheel.steeringAngle += 120.0f * deltaTime;
    frontLeftWheel.steeringAngle = glm::clamp(frontLeftWheel.steeringAngle, -45.0f, 45.0f);
    frontRightWheel.steeringAngle = glm::clamp(frontRightWheel.steeringAngle, -45.0f, 45.0f);
}

// Steer the car to the right by decreasing the steering angle and updating the wheel direction
void Car::steerRight(float deltaTime) {

    frontLeftWheel.steeringAngle -= 120.0f * deltaTime;
    frontRightWheel.steeringAngle -= 120.0f * deltaTime;
    frontLeftWheel.steeringAngle = glm::clamp(frontLeftWheel.steeringAngle, -45.0f, 45.0f);
    frontRightWheel.steeringAngle = glm::clamp(frontRightWheel.steeringAngle, -45.0f, 45.0f);

}

// Gradually center the steering and update the wheel direction accordingly
void Car::centerSteering(float deltaTime) {
    frontLeftWheel.steeringAngle = glm::mix(frontLeftWheel.steeringAngle, 0.0f, 2.0f * deltaTime);
    frontRightWheel.steeringAngle = glm::mix(frontRightWheel.steeringAngle, 0.0f, 2.0f * deltaTime);
}

void Car::updatePositionAndDirection(float deltaTime) {
    // Update the car's direction based on the rotation (yaw)
    direction = glm::vec3(sin(glm::radians(rotation)), 0.0f, cos(glm::radians(rotation)));

    // If the car is moving, update its rotation and position
    if (speed != 0.0f) {
        rotation += glm::clamp(frontLeftWheel.steeringAngle, -45.0f, 45.0f) * deltaTime;
        position += direction * speed * deltaTime;
    }
}

void Car::updateWheelRotations(float deltaTime) {

    float rotationSpeed = speed * deltaTime * 360.0f;

    backLeftWheel.rotation += rotationSpeed;
    backRightWheel.rotation += rotationSpeed;
    frontLeftWheel.rotation += rotationSpeed;
    frontRightWheel.rotation += rotationSpeed;

}