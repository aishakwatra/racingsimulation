#ifndef COLLISION_CHECKER_H
#define COLLISION_CHECKER_H

#include <glm/glm.hpp>
#include <vector>

struct Triangle {
    glm::vec3 v0, v1, v2;
};

class CollisionChecker {
public:

    CollisionChecker();

    void setGrid(const std::vector<std::vector<std::vector<Triangle>>>& gridCells, float gridSize, int gridWidth, int gridHeight);

    bool checkTrackIntersectionWithGrid(glm::vec3 rayOrigin, glm::vec3 rayDirection, glm::vec3& intersectionPoint);

private:
    bool intersectRayWithTriangle(glm::vec3 rayOrigin, glm::vec3 rayDirection, glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, float& t);

    const std::vector<std::vector<std::vector<Triangle>>>* gridCells;

    int gridSize = 0;
    int gridWidth = 0;
    int gridHeight = 0;

};

#endif