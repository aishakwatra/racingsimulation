#pragma once
#ifndef CAR_H
#define CARL_H

#include "Wheel.h"
#include "CollisionChecker.h"
#include <vector>
#include "Carconfig.h"
#include <iostream>


class Car {
public:
    explicit Car(const CarConfig& config);
    void applyConfig(const CarConfig& config);

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

    void setCollisionGrid(const std::vector<std::vector<Triangle>>& gridCells,const std::vector<std::vector<Triangle>>& gridCellsCollision, float gridSize, int gridWidth, int gridHeight);

    // Getters for position, direction, speed, and other properties
    glm::vec3 getPosition() const;
    glm::vec3 getDirection() const;
    float getSpeed() const;
    float getRotation() const;
    float getMaxSpeed() const;


private:
    glm::vec3 position;
    glm::vec3 bodyOffset;
    glm::vec3 direction;
    glm::vec3 bodyScale;
    glm::vec3 wheelScale;

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

    // Wheel rays (pointing downward)
    glm::vec3 frontLeftWheelRayOrigin, frontRightWheelRayOrigin;
    glm::vec3 backLeftWheelRayOrigin, backRightWheelRayOrigin;

    glm::vec3 frontLeftWheelIntersection, frontRightWheelIntersection;
    glm::vec3 backLeftWheelIntersection, backRightWheelIntersection;

    glm::vec3 downwardRayDirection = glm::vec3(0.0f, -1.0f, 0.0f);

    // Side rays (pointing outward)
    glm::vec3 frontRayOrigin, backRayOrigin;
    glm::vec3 leftSideRayOrigin, rightSideRayOrigin;

    glm::vec3 frontIntersection, backIntersection;
    glm::vec3 leftSideIntersection, rightSideIntersection;

    CollisionChecker collisionChecker;

    // Movement control flags
    bool canMoveForward = true;
    bool canMoveBackward = true;
    const float minCollisionDistance = 2.5f;

    AABB sideCollisionAABB;  // AABB for detecting side collisions

    // Method to compute the side collision AABB based on car size and position
    AABB computeSideAABB(const glm::vec3& nextPosition) const;

    glm::vec3 nextPosition;  // Store the next position for CCD


};


#endif