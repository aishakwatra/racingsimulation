#include "Car.h"

Car::Car(const CarConfig& config)
    : position(config.position),bodyOffset(config.bodyOffset),bodyScale(config.bodyScale), direction(config.direction), rotation(config.rotation),
    speed(config.speed), maxSpeed(config.maxSpeed), acceleration(config.acceleration),
    brakingForce(config.brakingForce), wheelScale(config.wheelScale), frontLeftWheel(config.frontLeftWheelOffset ,true),
    frontRightWheel(config.frontRightWheelOffset ), backLeftWheel(config.backLeftWheelOffset ,true),
    backRightWheel(config.backRightWheelOffset ),
    modelMatrix(glm::mat4(1.0f)) {}

void Car::applyConfig(const CarConfig& config) {
    position = config.position;
    bodyScale = config.bodyScale;
    bodyOffset = config.bodyOffset;
    wheelScale = config.wheelScale;
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
void Car::setCollisionGrid(const std::vector<std::vector<Triangle>>& gridCells,const std::vector<std::vector<Triangle>>& gridCellsCollision, float gridSize, int gridWidth, int gridHeight) {
    collisionChecker.setGrid(gridCells, gridCellsCollision, gridSize, gridWidth, gridHeight);
}

void Car::updateModelMatrix() {

    sideCollisionAABB = computeSideAABB(nextPosition);

    bool sideCollision = collisionChecker.checkTrackIntersectionWithGrid(sideCollisionAABB);

    if (sideCollision) {
        speed = 0.0f;
    } else {
        position = nextPosition;
    }

    // Offsets for wheel rays (pointing downward)
    glm::vec3 frontLeftWheelOffset = glm::vec3(-0.65f, 1.0f, 0.85f);
    glm::vec3 frontRightWheelOffset = glm::vec3(0.65f, 1.0f, 0.85f);
    glm::vec3 backLeftWheelOffset = glm::vec3(-0.65f, 1.0f, -0.85f);
    glm::vec3 backRightWheelOffset = glm::vec3(0.65f, 1.0f, -0.85f);

    // Offsets for side rays (pointing outward)
    glm::vec3 frontOffset(0.0f, 0.5f, 1.2f);
    glm::vec3 backOffset(0.0f, 0.5f, -1.2f);
    glm::vec3 leftOffset(-1.2f, 0.5f, 0.0f);
    glm::vec3 rightOffset(1.2f, 0.5f, 0.0f);


    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.0f));

    frontLeftWheelRayOrigin = position + glm::vec3(rotationMatrix * glm::vec4(frontLeftWheelOffset, 1.0f));
    frontRightWheelRayOrigin = position + glm::vec3(rotationMatrix * glm::vec4(frontRightWheelOffset, 1.0f));
    backLeftWheelRayOrigin = position + glm::vec3(rotationMatrix * glm::vec4(backLeftWheelOffset, 1.0f));
    backRightWheelRayOrigin = position + glm::vec3(rotationMatrix * glm::vec4(backRightWheelOffset, 1.0f));

    frontRayOrigin = position + glm::vec3(rotationMatrix * glm::vec4(frontOffset, 1.0f));
    backRayOrigin = position + glm::vec3(rotationMatrix * glm::vec4(backOffset, 1.0f));
    leftSideRayOrigin = position + glm::vec3(rotationMatrix * glm::vec4(leftOffset, 1.0f));
    rightSideRayOrigin = position + glm::vec3(rotationMatrix * glm::vec4(rightOffset, 1.0f));

    //find wheel collisions

    collisionChecker.checkTrackIntersectionWithGrid(frontLeftWheelRayOrigin, downwardRayDirection, frontLeftWheelIntersection);
    collisionChecker.checkTrackIntersectionWithGrid(frontRightWheelRayOrigin, downwardRayDirection, frontRightWheelIntersection);
    collisionChecker.checkTrackIntersectionWithGrid(backLeftWheelRayOrigin, downwardRayDirection, backLeftWheelIntersection);
    collisionChecker.checkTrackIntersectionWithGrid(backRightWheelRayOrigin, downwardRayDirection, backRightWheelIntersection);

    //update car orientation
    glm::vec3 midFront = (frontLeftWheelIntersection + frontRightWheelIntersection) / 2.0f;
    glm::vec3 midBack = (backLeftWheelIntersection + backRightWheelIntersection) / 2.0f;

    float rollHeightDifference = ((frontRightWheelIntersection.y + backRightWheelIntersection.y) / 2.0f) - ((frontLeftWheelIntersection.y + backLeftWheelIntersection.y) / 2.0f);
    float pitchHeightDifference = ((frontLeftWheelIntersection.y + frontRightWheelIntersection.y) / 2.0f) - ((backLeftWheelIntersection.y + backRightWheelIntersection.y) / 2.0f);

    // compute pitch and roll
    float pitchAngle = glm::atan(-pitchHeightDifference / glm::length(midFront - midBack));
    float rollAngle = glm::atan(rollHeightDifference / glm::length(frontRightWheelIntersection - frontLeftWheelIntersection));

    // update car's y-position
    position.y = (frontLeftWheelIntersection.y + frontRightWheelIntersection.y + backLeftWheelIntersection.y + backRightWheelIntersection.y) / 4.0f + 1.5f;

    // Update the model matrix
    modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, position);
    modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.0f));  // Yaw (left and right)
    modelMatrix = glm::rotate(modelMatrix, pitchAngle, glm::vec3(1.0f, 0.0f, 0.0f));  // Pitch (up/down)
    modelMatrix = glm::rotate(modelMatrix, rollAngle, glm::vec3(0.0f, 0.0f, 1.0f));  // Roll (side-to-side)
   
    // Update wheels' model matrices
    frontLeftWheel.updateModelMatrix(modelMatrix,wheelScale, true);
    frontRightWheel.updateModelMatrix(modelMatrix, wheelScale,true);
    backLeftWheel.updateModelMatrix(modelMatrix, wheelScale, false);
    backRightWheel.updateModelMatrix(modelMatrix, wheelScale, false);

}

glm::mat4 Car::getModelMatrix() const {
    glm::mat4 bodyModelMatrix = glm::translate(modelMatrix, bodyOffset);
    bodyModelMatrix = glm::scale(bodyModelMatrix, bodyScale);
    return bodyModelMatrix;
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
    if (canMoveForward) {
        speed += acceleration * deltaTime;
        if (speed > maxSpeed) {
            speed = maxSpeed;
        }
    }

}

void Car::brake(float deltaTime) {
    if (canMoveBackward) {
        speed -= brakingForce * deltaTime;
        if (speed < -maxSpeed / 2.0f) {
            speed = -maxSpeed / 2.0f;  // Limit reverse speed
        }
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

    // If the car is moving, update its rotation
    if (speed != 0.0f) {
        rotation += glm::clamp(frontLeftWheel.steeringAngle, -45.0f, 45.0f) * deltaTime;

        // Predict the next position
        nextPosition = position + direction * speed * deltaTime;
    }
    else {
        nextPosition = position;  // No movement if the car is not moving
    }


}

void Car::updateWheelRotations(float deltaTime) {

    float rotationSpeed = speed * deltaTime * 360.0f;

    backLeftWheel.rotation += rotationSpeed;
    backRightWheel.rotation += rotationSpeed;
    frontLeftWheel.rotation += rotationSpeed;
    frontRightWheel.rotation += rotationSpeed;

}


AABB Car::computeSideAABB(const glm::vec3& nextPosition) const {

    // half the size of the car's bounding box
    glm::vec3 halfSize(1.0f, 0.5f, 2.0f);  //width, height, length 

    glm::vec3 min = nextPosition - halfSize;
    glm::vec3 max = nextPosition + halfSize;

    return { min, max };

}