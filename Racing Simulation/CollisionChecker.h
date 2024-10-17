#ifndef COLLISION_CHECKER_H
#define COLLISION_CHECKER_H

#include <glm/glm.hpp>
#include <vector>

struct Triangle {
    glm::vec3 v0, v1, v2;
};

//Axis aligned bounding boxes
struct AABB {
    glm::vec3 min;  // Minimum corner of the box
    glm::vec3 max;  // Maximum corner of the box

    bool intersects(const AABB& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
            (min.y <= other.max.y && max.y >= other.min.y) &&
            (min.z <= other.max.z && max.z >= other.min.z);
    }
};

class CollisionChecker {
public:

    CollisionChecker();

    void setGrid(const std::vector<std::vector<std::vector<Triangle>>>& gridCells, float gridSize, int gridWidth, int gridHeight);
    bool checkTrackIntersectionWithGrid(glm::vec3 rayOrigin, glm::vec3 rayDirection, glm::vec3& intersectionPoint);
    bool checkTrackIntersectionWithGrid(const AABB& aabb);

private:

    bool overlapOnAxis(const glm::vec3& aabbHalfSize, const glm::vec3& axis, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2);
    bool intersectRayWithTriangle(glm::vec3 rayOrigin, glm::vec3 rayDirection, glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, float& t);
    bool intersectAABBWithTriangle(const AABB& aabb, const Triangle& tri);
    const std::vector<std::vector<std::vector<Triangle>>>* gridCells;


    int gridSize = 0;
    int gridWidth = 0;
    int gridHeight = 0;

};

#endif