#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 2.5f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;

class Camera {
public:
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    glm::vec3 CarPosition;  // Position of the car to follow
    float Yaw;
    float Pitch;
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;
    bool isDragging;
    float cameraLerpSpeed = 5.0f;
    float OrbitRadius;
    float OrbitMinRadius = 5.0f;
    float OrbitMaxRadius = 20.0f;

    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH, float orbitRadius = 10.0f)
        : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM), OrbitRadius(orbitRadius), isDragging(false) {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    glm::mat4 GetViewMatrix() {
        return glm::lookAt(Position, Position + Front, Up);
    }

    void ProcessKeyboard(Camera_Movement direction, float deltaTime) {
        if (isDragging) return;  // Ignore keyboard input while dragging
        float velocity = MovementSpeed * deltaTime;
        if (direction == FORWARD)
            Position += Front * velocity;
        if (direction == BACKWARD)
            Position -= Front * velocity;
        if (direction == LEFT)
            Position -= Right * velocity;
        if (direction == RIGHT)
            Position += Right * velocity;
    }

    void FollowCar(glm::vec3 carPosition, glm::vec3 carDirection, float carSpeed, float maxSpeed, float minOffset = 8.0f, float maxOffset = 15.0f) {
        if (isDragging) return;  // Do not adjust position while dragging

        CarPosition = carPosition;  // Update car position for later use

        float dynamicOffsetZ = glm::mix(minOffset, maxOffset, glm::clamp(carSpeed / maxSpeed, 0.0f, 1.0f));
        float cameraHeightOffset = 3.0f;

        Position = carPosition - carDirection * dynamicOffsetZ + glm::vec3(0.0f, cameraHeightOffset, 0.0f);

        Front = glm::normalize(carPosition + glm::vec3(0.0f, 1.0f, 0.0f) - Position);
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
    }

    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true) {
        if (!isDragging) return;  // Only rotate camera if dragging is active

        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw += xoffset;
        Pitch += yoffset;

        if (constrainPitch) {
            Pitch = glm::clamp(Pitch, -89.0f, 89.0f);
        }

        // Update the camera's position based on the latest yaw and pitch
        updateCameraPosition();
    }

    void updateCameraPosition() {
        // Spherical to Cartesian coordinates conversion
        glm::vec3 offset;
        offset.x = OrbitRadius * cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        offset.y = OrbitRadius * sin(glm::radians(Pitch));
        offset.z = OrbitRadius * sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Position = CarPosition + offset;

        // Re-calculate the Front vector
        Front = glm::normalize(CarPosition - Position);
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
    }

    void StartDragging() {
        isDragging = true;
    }

    void StopDragging() {
        isDragging = false;
        ResetToFollowCar();
    }

    void ResetToFollowCar() {
        // Reset the camera to follow the car smoothly
        FollowCar(CarPosition, glm::normalize(Position - CarPosition), 0, 1);  // Assuming carSpeed and maxSpeed are not critical here
    }

    void ProcessMouseScroll(float yoffset) {
        OrbitRadius -= (float)yoffset;
        OrbitRadius = glm::clamp(OrbitRadius, OrbitMinRadius, OrbitMaxRadius);

        updateCameraPosition();  // Update camera position based on new radius
    }

private:
    void updateCameraVectors() {
        // Calculate the front vector
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        // Recalculate the Right and Up vector
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
    }
};
#endif
