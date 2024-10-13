#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


#include "shader_m.h"
#include "camera.h"
#include "model.h"
#include "irrKlang/irrKlang.h"

#include <iostream>

using namespace irrklang;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

void handleCarSound();

void assignTrianglesToGrid(const Model& trackModel, float gridSize, int gridWidth, int gridHeight);
void checkTrackIntersectionWithGrid(glm::vec3 rayOrigin, glm::vec3 rayDirection, float gridSize, int gridWidth, int gridHeight, float& closestT, glm::vec3& intersectionPoint);
bool intersectRayWithTriangle(glm::vec3 rayOrigin, glm::vec3 rayDirection, glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, float& t);

// settings
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

// camera
Camera camera(glm::vec3(0.0f, 20.0f, 20.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

glm::vec3 rayOrigin;
glm::vec3 rayDirection = glm::vec3(0.0f, -1.0f, 0.0f);


//track divided into 15x15 grid with each block being 20x20 in size
int gridWidth = 15;
int gridHeight = 15; 
float gridSize = 20.0f; 

ISoundEngine* soundEngine;

ISound* accelerationSound = nullptr;
bool soundPlaying = false;

struct Triangle {
    glm::vec3 v0, v1, v2;
};

std::vector<std::vector<std::vector<Triangle>>> gridCells(gridWidth, std::vector<std::vector<Triangle>>(gridHeight));

struct CarBody {
    glm::vec3 position;
    glm::vec3 direction;
    float rotation;
    float speed;
    float maxSpeed;
    float acceleration;
    float brakingForce;
    glm::mat4 modelMatrix;

    CarBody() :
        position(glm::vec3(0.0f, 1.5f, 0.0f)),
        direction(glm::vec3(0.0f, 0.0f, 1.0f)),
        rotation(0.0f),
        speed(0.0f),
        maxSpeed(80.0f),
        acceleration(11.0f),
        brakingForce(20.0f),
        modelMatrix(glm::mat4(1.0f)) {}

    void updateModelMatrix();

};

struct Wheel {
    glm::vec3 offset;
    glm::vec3 scale;
    glm::vec3 direction;
    float rotation;
    float steeringAngle;
    float maxSteeringAngle;
    glm::mat4 modelMatrix;

    Wheel(glm::vec3 offsetPos) :
        offset(offsetPos),
        scale(glm::vec3(1.0f, 1.0f, 1.0f)),
        direction(glm::vec3(0.0f, 0.0f, 1.0f)),
        rotation(0.0f),
        steeringAngle(0.0f),
        maxSteeringAngle(45.0f),
        modelMatrix(glm::mat4(1.0f)) {}

    void updateModelMatrix(const glm::mat4& carModelMatrix, bool isSteeringWheel) {
        modelMatrix = glm::mat4(1.0f); 
        modelMatrix = carModelMatrix; 
        modelMatrix = glm::translate(modelMatrix, offset); 
        if (isSteeringWheel) {
            modelMatrix = glm::rotate(modelMatrix, glm::radians(steeringAngle), glm::vec3(0.0f, 1.0f, 0.0f)); 
        }
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMatrix = glm::scale(modelMatrix, scale);
    }

};

void CarBody::updateModelMatrix() {
    
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
    float frontLeftT, frontRightT, backLeftT, backRightT;

    checkTrackIntersectionWithGrid(frontLeftRayOrigin, rayDirection, gridSize, gridWidth, gridHeight, frontLeftT, frontLeftIntersection);
    checkTrackIntersectionWithGrid(frontRightRayOrigin, rayDirection, gridSize, gridWidth, gridHeight, frontRightT, frontRightIntersection);
    checkTrackIntersectionWithGrid(backLeftRayOrigin, rayDirection, gridSize, gridWidth, gridHeight, backLeftT, backLeftIntersection);
    checkTrackIntersectionWithGrid(backRightRayOrigin, rayDirection, gridSize, gridWidth, gridHeight, backRightT, backRightIntersection);

    //update car orientation
  
    glm::vec3 midFront = (frontLeftIntersection + frontRightIntersection) / 2.0f;
    glm::vec3 midBack = (backLeftIntersection + backRightIntersection) / 2.0f;

    float rollHeightDifference = ((frontRightIntersection.y + backRightIntersection.y) / 2.0f) - ((frontLeftIntersection.y + backLeftIntersection.y) / 2.0f);
    float pitchHeightDifference = ((frontLeftIntersection.y + frontRightIntersection.y) / 2.0f) - ((backLeftIntersection.y + backRightIntersection.y) / 2.0f);

    //pitch angle based on the height difference from front to back
    float pitchAngle = glm::atan(-pitchHeightDifference / glm::length(midFront - midBack)); 

    //roll angle based on the height difference from left to right
    float rollAngle = glm::atan(rollHeightDifference / glm::length(frontRightIntersection - frontLeftIntersection));

    // Update car's y-position based on the average height of all four intersections
    position.y = (frontLeftIntersection.y + frontRightIntersection.y + backLeftIntersection.y + backRightIntersection.y) / 4.0f + 1.5f;

    // Update the model matrix
    modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, position);
    modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.0f));  // Yaw (left and right)
    modelMatrix = glm::rotate(modelMatrix, pitchAngle, glm::vec3(1.0f, 0.0f, 0.0f));  // Pitch (up/down tilting)
    modelMatrix = glm::rotate(modelMatrix, rollAngle, glm::vec3(0.0f, 0.0f, 1.0f));  // Roll (side-to-side tilting)

}

// Instantiate car body and wheels
CarBody carBody;
Wheel backLeftWheel(glm::vec3(-0.65f, -0.6f, -0.85f));
Wheel backRightWheel(glm::vec3(0.65f, -0.6f, -0.85f));
Wheel frontLeftWheel(glm::vec3(-0.65f, -0.6f, 0.85f));
Wheel frontRightWheel(glm::vec3(0.65f, -0.6f, 0.85f));

int main()
{

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Model Loading", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(false);

    soundEngine = createIrrKlangDevice();

    if (!soundEngine) {
        std::cout << "Could not start sound engine" << std::endl;
        return 0;
    }

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);


    // build and compile our shader zprogram
    // ------------------------------------
    Shader ourShader("Shaders/model/model_loading.vs", "Shaders/model/model_loading.fs");


    // load models
    // -----------
    Model trackModel("objects/racetrack/track3.obj");
    Model carModel("objects/jeep/car.obj");
    Model wheelModel("objects/jeep/wheel.obj");

    assignTrianglesToGrid(trackModel, gridSize, gridWidth, gridHeight);


    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {

        // per-frame time logic
        // --------------------

        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        camera.FollowCar(carBody.position, carBody.direction, carBody.speed, carBody.maxSpeed);

        glm::vec3 rayOrigin = carBody.position + glm::vec3(0.0f, 1.0f, 0.0f);  // Slightly above the car
        glm::vec3 rayDirection = glm::vec3(0.0f, -1.0f, 0.0f);

        // render
        // ------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // also clear the depth buffer now!

        // don't forget to enable shader before setting uniforms
        ourShader.use();


        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 200.0f);
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        //track
        glm::mat4 model = glm::mat4(1.0f);
        ourShader.setMat4("model", model);
        trackModel.Draw(ourShader);

        //car body
        carBody.updateModelMatrix();
        ourShader.setMat4("model", carBody.modelMatrix);
        carModel.Draw(ourShader);

        //wheels
        auto drawWheel = [&](Wheel& wheel, bool isSteeringWheel) {
            wheel.updateModelMatrix(carBody.modelMatrix, isSteeringWheel);
            ourShader.setMat4("model", wheel.modelMatrix);  
            wheelModel.Draw(ourShader);  
        };

        drawWheel(backLeftWheel, false);
        drawWheel(backRightWheel, false);
        drawWheel(frontLeftWheel, true);
        drawWheel(frontRightWheel, true);

        handleCarSound();

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    soundEngine->drop();

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}


void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float rotationSpeed = 120.0f;
    float movementSpeed = carBody.speed * deltaTime;

    bool acceleratePressed = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    bool brakePressed = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        carBody.speed += carBody.acceleration * deltaTime;
        if (carBody.speed > carBody.maxSpeed) carBody.speed = carBody.maxSpeed;

    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        carBody.speed -= carBody.brakingForce * deltaTime;
        if (carBody.speed < -carBody.maxSpeed / 2.0f) carBody.speed = -carBody.maxSpeed / 2.0f;
    }
    

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_S) == GLFW_RELEASE)
    {
        if (carBody.speed > 0)
        {
            carBody.speed -= carBody.acceleration * deltaTime;
            if (carBody.speed < 0) carBody.speed = 0;
        }
        else if (carBody.speed < 0)
        {
            carBody.speed += carBody.acceleration * deltaTime;
            if (carBody.speed > 0) carBody.speed = 0;
        }

    }

    //Steering
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        frontLeftWheel.steeringAngle += rotationSpeed * deltaTime;
        frontRightWheel.steeringAngle += rotationSpeed * deltaTime;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        frontLeftWheel.steeringAngle -= rotationSpeed * deltaTime;
        frontRightWheel.steeringAngle -= rotationSpeed * deltaTime;
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_D) == GLFW_RELEASE)
    {
        frontLeftWheel.steeringAngle = glm::mix(frontLeftWheel.steeringAngle, 0.0f, 2.0f * deltaTime);
        frontRightWheel.steeringAngle = glm::mix(frontRightWheel.steeringAngle, 0.0f, 2.0f * deltaTime);
    }

    frontLeftWheel.steeringAngle = glm::clamp(frontLeftWheel.steeringAngle, -frontLeftWheel.maxSteeringAngle, frontLeftWheel.maxSteeringAngle);
    frontRightWheel.steeringAngle = glm::clamp(frontRightWheel.steeringAngle, -frontRightWheel.maxSteeringAngle, frontRightWheel.maxSteeringAngle);

    frontLeftWheel.direction = glm::vec3(sin(glm::radians(frontLeftWheel.steeringAngle)), 0.0f, cos(glm::radians(frontLeftWheel.steeringAngle)));
    frontRightWheel.direction = glm::vec3(sin(glm::radians(frontRightWheel.steeringAngle)), 0.0f, cos(glm::radians(frontRightWheel.steeringAngle)));

    glm::vec3 avgWheelDirection = glm::normalize((frontLeftWheel.direction + frontRightWheel.direction) / 2.0f);

    carBody.direction = glm::vec3(sin(glm::radians(carBody.rotation)), 0.0f, cos(glm::radians(carBody.rotation)));

    if (carBody.speed != 0.0f)
    {
        carBody.rotation += glm::clamp(frontLeftWheel.steeringAngle, -frontLeftWheel.maxSteeringAngle, frontLeftWheel.maxSteeringAngle) * deltaTime;
        carBody.position += carBody.direction * carBody.speed * deltaTime;
    }
    
    backLeftWheel.rotation += carBody.speed * deltaTime * 360.0f;
    backRightWheel.rotation += carBody.speed * deltaTime * 360.0f;
    frontLeftWheel.rotation += carBody.speed * deltaTime * 360.0f;
    frontRightWheel.rotation += carBody.speed * deltaTime * 360.0f;
   
}


void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);

}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

glm::vec3 calculateTriangleNormal(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2) {
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
    return normal;
} 


void assignTrianglesToGrid(const Model& trackModel, float gridSize, int gridWidth, int gridHeight) {

    for (const Mesh& mesh : trackModel.meshes) {
        for (unsigned int i = 0; i < mesh.indices.size(); i += 3) {

            glm::vec3 v0 = mesh.vertices[mesh.indices[i]].Position;
            glm::vec3 v1 = mesh.vertices[mesh.indices[i + 1]].Position;
            glm::vec3 v2 = mesh.vertices[mesh.indices[i + 2]].Position;

            // min and maxx and z coordinates of the triangle
            float minX = std::min({ v0.x, v1.x, v2.x });
            float maxX = std::max({ v0.x, v1.x, v2.x });
            float minZ = std::min({ v0.z, v1.z, v2.z });
            float maxZ = std::max({ v0.z, v1.z, v2.z });

            //grid cells that the triangle overlaps (how far way the point is from the origin in terms of grid cells)
            int minGridX = static_cast<int>(floor(minX / gridSize));
            int maxGridX = static_cast<int>(floor(maxX / gridSize));
            int minGridZ = static_cast<int>(floor(minZ / gridSize));
            int maxGridZ = static_cast<int>(floor(maxZ / gridSize));

            // Clamp grid coordinates to ensure they stay within the grid boundaries
            minGridX = std::max(0, std::min(gridWidth - 1, minGridX));
            maxGridX = std::max(0, std::min(gridWidth - 1, maxGridX));
            minGridZ = std::max(0, std::min(gridHeight - 1, minGridZ));
            maxGridZ = std::max(0, std::min(gridHeight - 1, maxGridZ));

            Triangle tri = { v0, v1, v2 };

            //loop all grid cells the triangle overlaps
            for (int x = minGridX; x <= maxGridX; x++) {
                for (int z = minGridZ; z <= maxGridZ; z++) {
                    gridCells[x][z].push_back(tri);
                }
            }
        }
    }
}

bool intersectRayWithTriangle(glm::vec3 rayOrigin, glm::vec3 rayDirection,glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, float& t) {

    const float EPSILON = 0.0000001f;
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;

    glm::vec3 h = glm::cross(rayDirection, edge2);
    float a = glm::dot(edge1, h);

    if (a > -EPSILON && a < EPSILON)
        return false;  // Ray is parallel to triangle

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
    return t > EPSILON;  // valid intersection


}

void checkTrackIntersectionWithGrid(glm::vec3 rayOrigin, glm::vec3 rayDirection, float gridSize, int gridWidth, int gridHeight, float& closestT, glm::vec3& intersectionPoint) {
    
    int carGridX = static_cast<int>(floor(rayOrigin.x / gridSize));
    int carGridZ = static_cast<int>(floor(rayOrigin.z / gridSize));

    carGridX = std::max(0, std::min(gridWidth - 1, carGridX));
    carGridZ = std::max(0, std::min(gridHeight - 1, carGridZ));

    int gridMinX = std::max(0, carGridX - 1);
    int gridMaxX = std::min(gridWidth - 1, carGridX + 1);
    int gridMinZ = std::max(0, carGridZ - 1);
    int gridMaxZ = std::min(gridHeight - 1, carGridZ + 1);

    closestT = std::numeric_limits<float>::max();

    for (int x = gridMinX; x <= gridMaxX; x++) {
        for (int z = gridMinZ; z <= gridMaxZ; z++) {

            for (const Triangle& tri : gridCells[x][z]) {
                float t;

                if (intersectRayWithTriangle(rayOrigin, rayDirection, tri.v0, tri.v1, tri.v2, t)) {
                    if (t < closestT) {
                        closestT = t;
                        intersectionPoint = rayOrigin + rayDirection * t;
                    }

                }
            }
        }
    }
}

void handleCarSound()
{

    static float fadeOutVolume = 1.0f;

    // Initialize the sound only once
    if (!accelerationSound && !soundPlaying)
    {
        accelerationSound = soundEngine->play2D("Sounds/accelerate_sound2.wav", true, true, true);
        accelerationSound->setVolume(0.0f);
        accelerationSound->setIsPaused(false);
        soundPlaying = true;

    }


    if (carBody.speed > 0.0f)
    {

        if (accelerationSound->getIsPaused())
        {
            accelerationSound->setPlaybackSpeed(1.0f);
            accelerationSound->setVolume(0.2f);
            fadeOutVolume = 1.0f;
            accelerationSound->setIsPaused(false);
        }

        float pitch = 1.0f + (carBody.speed / carBody.maxSpeed);
        accelerationSound->setPlaybackSpeed(pitch);

        float volume = glm::clamp(carBody.speed / carBody.maxSpeed, 0.2f, 1.0f);
        accelerationSound->setVolume(volume);

        fadeOutVolume = volume;

    }
    else if (carBody.speed == 0.0f && !accelerationSound->getIsPaused())
    {

        if (fadeOutVolume > 0.0f)
        {
            fadeOutVolume -= deltaTime * 0.5f; 
            fadeOutVolume = glm::clamp(fadeOutVolume, 0.0f, 1.0f); 
            accelerationSound->setVolume(fadeOutVolume);
        }
        else
        {
            accelerationSound->setIsPaused(true);
            accelerationSound->setPlaybackSpeed(1.0f);

        }
    }
}
