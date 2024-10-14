#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


#include "shader_m.h"
#include "Skybox.h"
#include "camera.h"
#include "model.h"
#include "irrKlang/irrKlang.h"

#include "Car.h" 
#include "Carconfig.h"


using namespace irrklang;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

void handleCarSound();

void assignTrianglesToGrid(const Model& trackModel, float gridSize, int gridWidth, int gridHeight, std::vector<std::vector<std::vector<Triangle>>>& gridCells);

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


CarConfig chevConfig;
Car car(chevConfig);


std::vector<std::vector<std::vector<Triangle>>> gridCells;


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
    Shader skyboxShader("Shaders/skybox/skybox.vs", "Shaders/skybox/skybox.fs");
        std::vector<std::string> faces = {
        "Textures/skybox/right.jpg",
        "Textures/skybox/left.jpg",
        "Textures/skybox/top.jpg",
        "Textures/skybox/bottom.jpg",
        "Textures/skybox/front.jpg",
        "Textures/skybox/back.jpg"
    };
    Skybox skybox(faces, skyboxShader.getID());
    Model trackModel("Objects/racetrack/track3.obj");

   //Model carModel("Objects/jeep/car.obj");
   //Model wheelModel("Objects/jeep/wheel.obj");
   Model carModel("Objects/chev-nascar/body.obj");
   Model wheelModel("Objects/chev-nascar/wheel1.obj");

    chevConfig.position = glm::vec3(0.0f, 0.0f, 0.0f);
    chevConfig.bodyOffset = glm::vec3(0.0f, -1.0f, 0.0f);
    chevConfig.bodyScale = glm::vec3(0.5f, 0.5f, 0.5f);
    chevConfig.wheelScale = glm::vec3(0.5f, 0.5f, 0.5f);
    chevConfig.maxSpeed = 120.0f;
    chevConfig.acceleration = 10.0f;
    chevConfig.brakingForce = 20.0f;
    chevConfig.frontRightWheelOffset = glm::vec3(-0.45f, -0.6f, 0.80f);
    chevConfig.frontLeftWheelOffset = glm::vec3(0.45f, -0.6f, 0.80f);
    chevConfig.backRightWheelOffset = glm::vec3(-0.45f, -0.6f, -1.00f);
    chevConfig.backLeftWheelOffset = glm::vec3(0.45f, -0.6f, -1.00f);

    car.applyConfig(chevConfig);
    assignTrianglesToGrid(trackModel, gridSize, gridWidth, gridHeight, gridCells);
    car.setCollisionGrid(gridCells, gridSize, gridWidth, gridHeight);

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

        camera.FollowCar(car.getPosition(), car.getDirection(), car.getSpeed(), car.getMaxSpeed());

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
        car.updateModelMatrix();  // Update the car and wheel transformations
        ourShader.setMat4("model", car.getModelMatrix());
        carModel.Draw(ourShader);

        ourShader.setMat4("model", car.getFrontLeftWheelModelMatrix());
        wheelModel.Draw(ourShader);

        ourShader.setMat4("model", car.getFrontRightWheelModelMatrix());
        wheelModel.Draw(ourShader);

        ourShader.setMat4("model", car.getBackLeftWheelModelMatrix());
        wheelModel.Draw(ourShader);

        ourShader.setMat4("model", car.getBackRightWheelModelMatrix());
        wheelModel.Draw(ourShader);

        skybox.draw(view, projection);

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


    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        car.accelerate(deltaTime);
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        car.brake(deltaTime);
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_S) == GLFW_RELEASE) {
        car.slowDown(deltaTime);
    }

    // Handle steering
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        car.steerLeft(deltaTime);
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        car.steerRight(deltaTime);
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_D) == GLFW_RELEASE) {
        car.centerSteering(deltaTime);
    }

    car.updatePositionAndDirection(deltaTime);

    car.updateWheelRotations(deltaTime);

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


void assignTrianglesToGrid(const Model& trackModel, float gridSize, int gridWidth, int gridHeight, std::vector<std::vector<std::vector<Triangle>>>& gridCells) {

    // Resize gridCells
    gridCells.resize(gridWidth, std::vector<std::vector<Triangle>>(gridHeight));

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


    if (car.getSpeed() > 0.0f)
    {

        if (accelerationSound->getIsPaused())
        {
            accelerationSound->setPlaybackSpeed(1.0f);
            accelerationSound->setVolume(0.2f);
            fadeOutVolume = 1.0f;
            accelerationSound->setIsPaused(false);
        }

        float pitch = 1.0f + (car.getSpeed() / car.getMaxSpeed());
        accelerationSound->setPlaybackSpeed(pitch);

        float volume = glm::clamp(car.getSpeed() / car.getMaxSpeed(), 0.2f, 1.0f);
        accelerationSound->setVolume(volume);

        fadeOutVolume = volume;

    }
    else if (car.getSpeed() == 0.0f && !accelerationSound->getIsPaused())
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