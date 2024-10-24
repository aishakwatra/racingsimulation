#include "Car.h"

const float SHARP_TURN_SPEED_THRESHOLD = 50.0f; // speed in km/h
const float SHARP_TURN_ANGLE_THRESHOLD = 30.0f; // angle in degrees
float currentPitch;
float currentRoll;
Car::Car(const CarConfig& config)
    : position(config.position),bodyOffset(config.bodyOffset),bodyScale(config.bodyScale), direction(config.direction), rotation(config.rotation),
    speed(config.speed), maxSpeed(config.maxSpeed), acceleration(config.acceleration), maxSteeringAngleAtMaxSpeed(config.maxSteeringAngleAtMaxSpeed),
    maxSteeringAngleAtZeroSpeed(config.maxSteeringAngleAtZeroSpeed), turnSharpnessFactor(config.turnSharpnessFactor),
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
    maxSteeringAngleAtMaxSpeed = config.maxSteeringAngleAtMaxSpeed;
    maxSteeringAngleAtZeroSpeed = config.maxSteeringAngleAtZeroSpeed;
    turnSharpnessFactor = config.turnSharpnessFactor;
    frontLeftWheel.setOffset(config.frontLeftWheelOffset);
    frontRightWheel.setOffset(config.frontRightWheelOffset);
    backLeftWheel.setOffset(config.backLeftWheelOffset);
    backRightWheel.setOffset(config.backRightWheelOffset);
}
void Car::setCollisionGrid(const std::vector<std::vector<Triangle>>& gridCells,const std::vector<std::vector<Triangle>>& gridCellsCollision, float gridSize, int gridWidth, int gridHeight) {
    collisionChecker.setGrid(gridCells, gridCellsCollision, gridSize, gridWidth, gridHeight);
}

void Car::updateModelMatrix() {

    sideCollisionAABB.update(modelMatrix);

    bool sideCollision = collisionChecker.checkTrackIntersectionWithGrid(sideCollisionAABB);

    if (sideCollision) {

        glm::vec3 correctionDirection = (speed >= 0.0f) ? -direction : direction;

        // Apply correction to position in the correct direction
        glm::vec3 correction = correctionDirection * 0.3f;
        position += correction;

        // Halt the car's movement by setting speed to 0
        speed *= 0.5f;

    }
    else {
        // Proceed to the next position if no collision occurs
        position = nextPosition;
    }

    // Offsets for wheel rays (pointing downward)
    glm::vec3 frontLeftWheelOffset = glm::vec3(-0.65f, 1.2f, 0.85f);
    glm::vec3 frontRightWheelOffset = glm::vec3(0.65f, 1.2f, 0.85f);
    glm::vec3 backLeftWheelOffset = glm::vec3(-0.65f, 1.2f, -0.85f);
    glm::vec3 backRightWheelOffset = glm::vec3(0.65f, 1.2f, -0.85f);

    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.0f));

    frontLeftWheelRayOrigin = position + glm::vec3(rotationMatrix * glm::vec4(frontLeftWheelOffset, 1.0f));
    frontRightWheelRayOrigin = position + glm::vec3(rotationMatrix * glm::vec4(frontRightWheelOffset, 1.0f));
    backLeftWheelRayOrigin = position + glm::vec3(rotationMatrix * glm::vec4(backLeftWheelOffset, 1.0f));
    backRightWheelRayOrigin = position + glm::vec3(rotationMatrix * glm::vec4(backRightWheelOffset, 1.0f));

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

   

    float pitchAngleTarget = glm::atan(-pitchHeightDifference / glm::length(midFront - midBack));
    float rollAngleTarget = glm::atan(rollHeightDifference / glm::length(frontRightWheelIntersection - frontLeftWheelIntersection));

    // Determine how fast the orientation should change based on speed
    float lerpFactor = glm::mix(1.0f, 0.05f, glm::clamp(speed / maxSpeed, 0.0f, 1.0f));  // Fast orientation change when slow, slow when fast

    
    // Interpolate between current orientation and target orientation (pitch and roll)
    currentPitch = glm::mix(currentPitch, pitchAngleTarget, lerpFactor);
    currentRoll = glm::mix(currentRoll, rollAngleTarget, lerpFactor);

    // Update car's y-position
    position.y = (frontLeftWheelIntersection.y + frontRightWheelIntersection.y + backLeftWheelIntersection.y + backRightWheelIntersection.y) / 4.0f + 1.5f;

    // Update the model matrix
    modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, position);
    modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.0f));  // Yaw (left and right)
    modelMatrix = glm::rotate(modelMatrix, currentPitch, glm::vec3(1.0f, 0.0f, 0.0f));  // Pitch (up/down)
    modelMatrix = glm::rotate(modelMatrix, currentRoll, glm::vec3(0.0f, 0.0f, 1.0f));  // Roll (side-to-side)
   
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

       speed += acceleration * deltaTime;
        if (speed > maxSpeed) {
            speed = maxSpeed;
        }
    

}

void Car::brake(float deltaTime) {

        speed -= brakingForce * deltaTime;
        if (speed < -maxSpeed / 2.0f) {
            speed = -maxSpeed / 2.0f;  // Limit reverse speed
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

bool Car::isSharpTurn(float steeringAngle) const {
    // Define a sharp turn as having a large steering angle at a high speed
    // You could make this more sophisticated by making the threshold speed-dependent
    return std::abs(steeringAngle) > SHARP_TURN_ANGLE_THRESHOLD && speed > SHARP_TURN_SPEED_THRESHOLD;
}

void Car::steerLeft(float deltaTime) {
    float angleChange = 120.0f * deltaTime; // Degrees per second
    frontLeftWheel.steeringAngle += angleChange;
    frontRightWheel.steeringAngle += angleChange;

    // Clamp the steering angle
    frontLeftWheel.steeringAngle = glm::clamp(frontLeftWheel.steeringAngle, -45.0f, 45.0f);
    frontRightWheel.steeringAngle = glm::clamp(frontRightWheel.steeringAngle, -45.0f, 45.0f);

    // Check if it's a sharp turn
    bool sharpTurn = isSharpTurn(frontLeftWheel.steeringAngle);
    if (sharpTurn) {
        // Adjust handling for sharp turn, e.g., reduce speed
        slowDown(deltaTime * 2); // Slow down faster if it's a sharp turn
    }
}

void Car::steerRight(float deltaTime) {
    float angleChange = -120.0f * deltaTime; // Degrees per second
    frontLeftWheel.steeringAngle += angleChange;
    frontRightWheel.steeringAngle += angleChange;

    // Clamp the steering angle
    frontLeftWheel.steeringAngle = glm::clamp(frontLeftWheel.steeringAngle, -45.0f, 45.0f);
    frontRightWheel.steeringAngle = glm::clamp(frontRightWheel.steeringAngle, -45.0f, 45.0f);

    // Check if it's a sharp turn
    bool sharpTurn = isSharpTurn(frontRightWheel.steeringAngle);
    if (sharpTurn) {
        // Adjust handling for sharp turn
        slowDown(deltaTime * 2); // Slow down faster if it's a sharp turn
    }
}

// Gradually center the steering and update the wheel direction accordingly
void Car::centerSteering(float deltaTime) {
    frontLeftWheel.steeringAngle = glm::mix(frontLeftWheel.steeringAngle, 0.0f, 2.0f * deltaTime);
    frontRightWheel.steeringAngle = glm::mix(frontRightWheel.steeringAngle, 0.0f, 2.0f * deltaTime);
}

float Car::getSteeringAngle() const {
    // Assuming you have some logic to compute or retrieve the steering angle
    // For example, if steering angle is simply the average of both front wheels:
    return (frontLeftWheel.steeringAngle + frontRightWheel.steeringAngle) / 2.0f;
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

    backLeftWheel.rotation -= rotationSpeed;
    backRightWheel.rotation += rotationSpeed;
    frontLeftWheel.rotation -= rotationSpeed;
    frontRightWheel.rotation += rotationSpeed;

}
