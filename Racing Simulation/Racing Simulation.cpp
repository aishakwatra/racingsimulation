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

#include "SoundManager.h"


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);

void processInput(GLFWwindow* window);

void handleCarSound(SoundManager& soundManager, const Car& car);

void assignTrianglesToGrid(const Model& trackModel, float gridSize, int gridWidth, int gridHeight, std::vector<std::vector<Triangle>>& gridCells);

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
float gridSize = 7.0f; 

CarConfig chevConfig;
Car car(chevConfig);

std::vector<std::vector<Triangle>> gridCells;

SoundManager soundManager;

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
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
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

    soundManager.preloadSound("accelerate", "Sounds/accelerate_sound2.wav");

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
        car.updatePositionAndDirection(deltaTime);
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

        handleCarSound(soundManager, car);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

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


void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    static float lastX = SCR_WIDTH / 2.0;
    static float lastY = SCR_HEIGHT / 2.0;
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Reverse since y-coordinates range from bottom to top
    lastX = xpos;
    lastY = ypos;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        camera.ProcessMouseMovement(xoffset, yoffset);
    }
}



void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            // When the left mouse button is pressed, start dragging
            camera.StartDragging();
        }
        else if (action == GLFW_RELEASE) {
            // When the left mouse button is released, stop dragging
            camera.StopDragging();
        }
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    static double lastX = xpos;
    static double lastY = ypos;

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    // Only update camera if dragging is active
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        camera.ProcessMouseMovement(xoffset, yoffset);
    }
}

// Function to be called whenever the mouse scroll wheel is used
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(yoffset);
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

void printGridCells() {
    int totalSum = 0;

    for (int i = 0; i < gridCells.size(); ++i) {

        int num = gridCells[i].size();  // Number of triangles in this cell
        std::cout << "Number of triangles: " << num << std::endl;

        totalSum += num;
    }

    std::cout << "TOTAL SUM: " << totalSum << std::endl;
}


int getGridIndex(int x, int z, int gridWidth) {
    return z * gridWidth + x;
}


void assignTrianglesToGrid(const Model& trackModel, float gridSize, int gridWidth, int gridHeight,std::vector<std::vector<Triangle>> & gridCells) {

    // Resize gridCells
    gridCells.resize(gridWidth * gridHeight);

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

            // Assign the triangle to the relevant grid cells
            for (int x = minGridX; x <= maxGridX; ++x) {
                for (int z = minGridZ; z <= maxGridZ; ++z) {
                    int index = getGridIndex(x, z, gridWidth);
                    gridCells[index].push_back(tri);
                }
            }
        }
    }

    printGridCells();

}



void handleCarSound(SoundManager& soundManager, const Car& car) {
    static float fadeOutVolume = 1.0f;

    // Check if the car is moving forward
    if (car.getSpeed() > 0.0f) {
        // If the sound is not playing, play it from the beginning
        if (!soundManager.isPlaying("accelerate")) {
            soundManager.stopSound("accelerate");  // Ensure the sound is reset
            soundManager.playSound("accelerate", true);  // Play looped
            soundManager.setVolume("accelerate", 0.2f);  // Start with a low volume
        }

        // Adjust pitch and volume based on speed
        float pitch = 1.0f + (car.getSpeed() / car.getMaxSpeed());
        soundManager.setPlaybackSpeed("accelerate", pitch);

        float volume = glm::clamp(car.getSpeed() / car.getMaxSpeed(), 0.2f, 1.0f);
        soundManager.setVolume("accelerate", volume);

        fadeOutVolume = volume;
    }
    else {  // If the car is stopped
        if (fadeOutVolume > 0.0f) {
            fadeOutVolume -= deltaTime * 0.5f;
            fadeOutVolume = glm::clamp(fadeOutVolume, 0.0f, 1.0f);
            soundManager.setVolume("accelerate", fadeOutVolume);
        }
        else {
            soundManager.stopSound("accelerate");  // Stop the sound when fully faded out
        }
    }
}