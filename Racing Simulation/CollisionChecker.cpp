#include "CollisionChecker.h"

// Custom Min and Max for float
float customMin(float a, float b) {
    return (a < b) ? a : b;
}

float customMax(float a, float b) {
    return (a > b) ? a : b;
}

// Custom Min and Max for int
int customMin(int a, int b) {
    return (a < b) ? a : b;
}

int customMax(int a, int b) {
    return (a > b) ? a : b;
}


CollisionChecker::CollisionChecker() : gridCells(nullptr) {}

void CollisionChecker::setGrid(const std::vector<std::vector<std::vector<Triangle>>>& externalGridCells, float gridSize, int gridWidth, int gridHeight) {
    gridCells = &externalGridCells;  // Store pointer to the external grid
    this->gridSize = gridSize;
    this->gridWidth = gridWidth;
    this->gridHeight = gridHeight;
}


bool CollisionChecker::checkTrackIntersectionWithGrid(glm::vec3 rayOrigin, glm::vec3 rayDirection, glm::vec3& intersectionPoint) {

    if (!gridCells) return false;

    int carGridX = static_cast<int>(std::floor(rayOrigin.x / gridSize));
    int carGridZ = static_cast<int>(std::floor(rayOrigin.z / gridSize));

    carGridX = customMin(gridWidth - 1, customMax(0, carGridX));
    carGridZ = customMin(gridHeight - 1, customMax(0, carGridZ));

    int gridMinX = customMax(0, carGridX - 1);
    int gridMaxX = customMin(gridWidth - 1, carGridX + 1);
    int gridMinZ = customMax(0, carGridZ - 1);
    int gridMaxZ = customMin(gridHeight - 1, carGridZ + 1);

    const float MAX_FLOAT = 3.402823466e+38F;  // Maximum float value
    float closestT = MAX_FLOAT;
    bool hasIntersection = false;

    for (int x = gridMinX; x <= gridMaxX; x++) {
        for (int z = gridMinZ; z <= gridMaxZ; z++) {
            for (const Triangle& tri : (*gridCells)[x][z]) {
                float t;
                if (intersectRayWithTriangle(rayOrigin, rayDirection, tri.v0, tri.v1, tri.v2, t)) {
                    if (t < closestT) {
                        closestT = t;
                        intersectionPoint = rayOrigin + rayDirection * t;
                        hasIntersection = true;
                    }
                }
            }
        }
    }

    return hasIntersection;
}

bool CollisionChecker::intersectRayWithTriangle(glm::vec3 rayOrigin, glm::vec3 rayDirection, glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, float& t) {

    const float EPSILON = 0.0000001f;
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;

    glm::vec3 h = glm::cross(rayDirection, edge2);
    float a = glm::dot(edge1, h);

    if (a > -EPSILON && a < EPSILON)
        return false;  // Ray is parallel to the triangle

    float f = 1.0f / a;
    glm::vec3 s = rayOrigin - v0;
    float u = f * glm::dot(s, h);
    if (u < 0.0f || u > 1.0f)
        return false;

    glm::vec3 q = glm::cross(s, edge1);
    float v = f * glm::dot(rayDirection, q);
    if (v < 0.0f || u + v > 1.0f)
        return false;

    t = f * glm::dot(edge2, q);
    return t > EPSILON;  // Valid intersection
}


