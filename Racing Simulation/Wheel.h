#ifndef WHEEL_H
#define WHEEL_H

#include <glm/gtc/matrix_transform.hpp>

class Wheel {
public:
    Wheel(const glm::vec3& offset);
    void updateModelMatrix(const glm::mat4& carModelMatrix, bool isSteeringWheel);
    glm::mat4 getModelMatrix() const;

    void setRotation(float rotation);
    void setOffset(const glm::vec3& offset);
    void setSteeringAngle(float steeringAngle);

    float rotation;
    float steeringAngle;

private:
    glm::vec3 offset;
    glm::mat4 modelMatrix;

    float maxSteeringAngle;

};

#endif

