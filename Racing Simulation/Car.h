#pragma once
#ifndef CAR_H
#define CARL_H

#include "Wheel.h"
#include "CollisionChecker.h"
#include <vector>

class Car {
public:
    Car();

    void updateModelMatrix();
    glm::mat4 getModelMatrix() const;

    // Accessors for wheels' matrices
    glm::mat4 getFrontLeftWheelModelMatrix() const;
    glm::mat4 getFrontRightWheelModelMatrix() const;
    glm::mat4 getBackLeftWheelModelMatrix() const;
    glm::mat4 getBackRightWheelModelMatrix() const;

    // Movement and steering methods
    void accelerate(float deltaTime);
    void brake(float deltaTime);
    void slowDown(float deltaTime);
    void steerLeft(float deltaTime);
    void steerRight(float deltaTime);
    void centerSteering(float deltaTime);
    void updateWheelRotations(float deltaTime);
    void updatePositionAndDirection(float deltaTime);

    void setCollisionGrid(const std::vector<std::vector<std::vector<Triangle>>>& gridCells, float gridSize, int gridWidth, int gridHeight);

    // Getters for position, direction, speed, and other properties
    glm::vec3 getPosition() const;
    glm::vec3 getDirection() const;
    float getSpeed() const;
    float getRotation() const;
    float getMaxSpeed() const;

    CollisionChecker collisionChecker;

private:
    glm::vec3 position;
    glm::vec3 direction;
    float rotation;
    float speed;
    float maxSpeed;
    float acceleration;
    float brakingForce;
    glm::mat4 modelMatrix;

    Wheel frontLeftWheel;
    Wheel frontRightWheel;
    Wheel backLeftWheel;
    Wheel backRightWheel;
};


#endif