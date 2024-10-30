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

float calculateOptimalGridSize(const Model& trackModel, int desiredGridCount);
void assignTrianglesToGrid(const Model& trackModel, float gridSize, int gridWidth, int gridHeight, std::vector<std::vector<Triangle>>& gridCells);
void checkTrackSize(const Model& trackModel);

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
int gridWidth = 8;
int gridHeight = 8;
float gridSize = 0.0f;

CarConfig chevConfig;
Car car(chevConfig);

std::vector<std::vector<Triangle>> gridCells;
std::vector<std::vector<Triangle>> gridCellsCollision;

SoundManager soundManager;


glm::vec3 lightPositions[4] = {
glm::vec3(10.0f, 5.0f, 10.0f),
glm::vec3(-10.0f, 5.0f, 10.0f),
glm::vec3(10.0f, 5.0f, -10.0f),
glm::vec3(-10.0f, 5.0f, -10.0f)
};

glm::vec3 lightColors[4] = {
    glm::vec3(500.0f, 500.0f, 500.0f),
    glm::vec3(500.0f, 500.0f, 500.0f),
    glm::vec3(500.0f, 500.0f, 500.0f),
    glm::vec3(500.0f, 500.0f, 500.0f)
};


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
    Shader pbrShader("Shaders/PBR/pbr.vs", "Shaders/PBR/pbr.fs");
    Shader equirectangularToCubemapShader("Shaders/PBR/cubemap.vs", "Shaders/PBR/equirectangular_to_cubemap.fs");
    Shader irradianceShader("Shaders/PBR/cubemap.vs", "Shaders/PBR/irradiance_convolution.fs");
    Shader prefilterShader("Shaders/PBR/cubemap.vs", "Shaders/PBR/prefilter.fs");
    Shader brdfShader("Shaders/PBR/brdf.vs", "Shaders/PBR/brdf.fs");

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
    Model trackCollisionModel("Objects/racetrack/track.obj");

    //Model carModel("Objects/jeep/car.obj");
    //Model wheelModel("Objects/jeep/wheel.obj");
    //Model carModel("Objects/chev-nascar/body.obj");
    Model wheelModel("Objects/chev-nascar/wheel1.obj");
    Model carModel("Objects/pbrCar/CarBody2.obj");
    ourShader.use();
    ourShader.setInt("material.diffuse", 0);
    ourShader.setInt("material.specular", 1);

    pbrShader.use();
    pbrShader.setInt("irradianceMap", 0);
    pbrShader.setInt("prefilterMap", 1);
    pbrShader.setInt("brdfLUT", 2);
    pbrShader.setInt("albedoMap", 3);
    pbrShader.setInt("normalMap", 4);
    pbrShader.setInt("metallicMap", 5);
    pbrShader.setInt("roughnessMap", 6);
    pbrShader.setInt("aoMap", 7);

    // pbr: setup framebuffer
   // ----------------------
    unsigned int captureFBO;
    unsigned int captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

    // pbr: load the HDR environment map
    // ---------------------------------
    stbi_set_flip_vertically_on_load(true);
    int width, height, nrComponents;
    float* data = stbi_loadf("Textures/track_hdr.hdr", & width, & height, & nrComponents, 0);
    unsigned int hdrTexture;
    if (data)
    {
        glGenTextures(1, &hdrTexture);
        glBindTexture(GL_TEXTURE_2D, hdrTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data); // note how we specify the texture's data value to be float

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Failed to load HDR image." << std::endl;
    }

    // pbr: setup cubemap to render to and attach to framebuffer
    // ---------------------------------------------------------
    unsigned int envCubemap;
    glGenTextures(1, &envCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // enable pre-filter mipmap sampling (combatting visible dots artifact)
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // pbr: set up projection and view matrices for capturing data onto the 6 cubemap face directions
    // ----------------------------------------------------------------------------------------------
    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] =
    {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };

    // pbr: convert HDR equirectangular environment map to cubemap equivalent
    // ----------------------------------------------------------------------
    equirectangularToCubemapShader.use();
    equirectangularToCubemapShader.setInt("equirectangularMap", 0);
    equirectangularToCubemapShader.setMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrTexture);

    glViewport(0, 0, 512, 512); // don't forget to configure the viewport to the capture dimensions.
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    for (unsigned int i = 0; i < 6; ++i)
    {
        equirectangularToCubemapShader.setMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //renderCube();

    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // then let OpenGL generate mipmaps from first mip face (combatting visible dots artifact)
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    // pbr: create an irradiance cubemap, and re-scale capture FBO to irradiance scale.
    // --------------------------------------------------------------------------------
    unsigned int irradianceMap;
    glGenTextures(1, &irradianceMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

    // pbr: solve diffuse integral by convolution to create an irradiance (cube)map.
    // -----------------------------------------------------------------------------
    irradianceShader.use();
    irradianceShader.setInt("environmentMap", 0);
    irradianceShader.setMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

    glViewport(0, 0, 32, 32); // don't forget to configure the viewport to the capture dimensions.
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    for (unsigned int i = 0; i < 6; ++i)
    {
        irradianceShader.setMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //renderCube();

    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // pbr: create a pre-filter cubemap, and re-scale capture FBO to pre-filter scale.
    // --------------------------------------------------------------------------------
    unsigned int prefilterMap;
    glGenTextures(1, &prefilterMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // be sure to set minification filter to mip_linear 
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // generate mipmaps for the cubemap so OpenGL automatically allocates the required memory.
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    // pbr: run a quasi monte-carlo simulation on the environment lighting to create a prefilter (cube)map.
    // ----------------------------------------------------------------------------------------------------
    prefilterShader.use();
    prefilterShader.setInt("environmentMap", 0);
    prefilterShader.setMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    unsigned int maxMipLevels = 5;
    for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
    {
        // reisze framebuffer according to mip-level size.
        unsigned int mipWidth = static_cast<unsigned int>(128 * std::pow(0.5, mip));
        unsigned int mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mip));
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = (float)mip / (float)(maxMipLevels - 1);
        prefilterShader.setFloat("roughness", roughness);
        for (unsigned int i = 0; i < 6; ++i)
        {
            prefilterShader.setMat4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            //renderCube();
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // pbr: generate a 2D LUT from the BRDF equations used.
    // ----------------------------------------------------
    unsigned int brdfLUTTexture;
    glGenTextures(1, &brdfLUTTexture);

    // pre-allocate enough memory for the LUT texture.
    glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
    // be sure to set wrapping mode to GL_CLAMP_TO_EDGE
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // then re-configure capture framebuffer object and render screen-space quad with BRDF shader.
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);

    glViewport(0, 0, 512, 512);
    brdfShader.use();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //renderQuad();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

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

    gridSize = calculateOptimalGridSize(trackModel, gridHeight);

    car.applyConfig(chevConfig);
    assignTrianglesToGrid(trackCollisionModel, gridSize, gridWidth, gridHeight, gridCells);
    assignTrianglesToGrid(trackModel, gridSize, gridWidth, gridHeight, gridCellsCollision);
    car.setCollisionGrid(gridCells, gridCellsCollision, gridSize, gridWidth, gridHeight);

    soundManager.preloadSound("accelerate", "Sounds/accelerate_sound2.wav");

    pbrShader.use();

    for (int i = 0; i < 4; ++i) {
        pbrShader.setVec3("lightPositions[" + std::to_string(i) + "]", lightPositions[i]);
        pbrShader.setVec3("lightColors[" + std::to_string(i) + "]", lightColors[i]);
    }

    int scrWidth, scrHeight;
    glfwGetFramebufferSize(window, &scrWidth, &scrHeight);
    glViewport(0, 0, scrWidth, scrHeight);

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
        
        pbrShader.use();

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 200.0f);
        glm::mat4 view = camera.GetViewMatrix();
        pbrShader.setMat4("projection", projection);
        pbrShader.setMat4("view", view);
        pbrShader.setVec3("camPos", camera.Position);

        //track
        glm::mat4 model = glm::mat4(1.0f);
        pbrShader.setMat4("model", model);
        pbrShader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);

        trackModel.Draw(pbrShader);

        //car body
        car.updatePositionAndDirection(deltaTime);
        car.updateModelMatrix();  // Update the car and wheel transformations
        pbrShader.setMat4("model", car.getModelMatrix());
        pbrShader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(car.getModelMatrix()))));
        carModel.Draw(pbrShader);

        pbrShader.setMat4("model", car.getFrontLeftWheelModelMatrix());
        pbrShader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(car.getFrontLeftWheelModelMatrix()))));
        wheelModel.Draw(pbrShader);

        pbrShader.setMat4("model", car.getFrontRightWheelModelMatrix());
        pbrShader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(car.getFrontLeftWheelModelMatrix()))));
        wheelModel.Draw(pbrShader);

        pbrShader.setMat4("model", car.getBackLeftWheelModelMatrix());
        pbrShader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(car.getFrontLeftWheelModelMatrix()))));
        wheelModel.Draw(pbrShader);

        pbrShader.setMat4("model", car.getBackRightWheelModelMatrix());
        pbrShader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(car.getFrontLeftWheelModelMatrix()))));
        wheelModel.Draw(pbrShader);

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
        std::cout << "Grid Cell " << i << std::endl;
        std::cout << "Number of triangles: " << num << std::endl;

        totalSum += num;
    }

    std::cout << "TOTAL SUM: " << totalSum << std::endl;
}


int getGridIndex(int x, int z, int gridWidth) {
    return z * gridWidth + x;
}


void assignTrianglesToGrid(const Model& trackModel, float gridSize, int gridWidth, int gridHeight, std::vector<std::vector<Triangle>>& gridCells) {

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

    //printGridCells();

}

void checkTrackSize(const Model& trackModel) {

    int minX = 0;
    int maxX = 0;

    for (const Mesh& mesh : trackModel.meshes) {
        for (unsigned int i = 0; i < mesh.indices.size(); i += 3) {

            glm::vec3 v0 = mesh.vertices[mesh.indices[i]].Position;
            glm::vec3 v1 = mesh.vertices[mesh.indices[i + 1]].Position;
            glm::vec3 v2 = mesh.vertices[mesh.indices[i + 2]].Position;

            if (v0.x < minX) minX = v0.x;
            if (v1.x < minX) minX = v1.x;
            if (v2.x < minX) minX = v2.x;

            if (v0.x > maxX) maxX = v0.x;
            if (v1.x > maxX) maxX = v1.x;
            if (v2.x > maxX) maxX = v2.x;

        }
    }

    std::cout << "Minimum X: " << minX << "Maximum Y" << maxX << std::endl;


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


float calculateOptimalGridSize(const Model& trackModel, int desiredGridCount) {

    // Initialize min and max bounds
    glm::vec3 minBounds(FLT_MAX, FLT_MAX, FLT_MAX);
    glm::vec3 maxBounds(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (const Mesh& mesh : trackModel.meshes) {
        for (unsigned int i = 0; i < mesh.indices.size(); i++) {

            glm::vec3 vertex = mesh.vertices[mesh.indices[i]].Position;

            minBounds = glm::min(minBounds, vertex);
            maxBounds = glm::max(maxBounds, vertex);

        }
    }

    // Calculate the dimensions of the bounding box for the entire track
    glm::vec3 trackSize = maxBounds - minBounds;

    // Find the maximum extent along the X and Z axes (for 2D grid division)
    float maxDimension = glm::max(trackSize.x, trackSize.z);

    // Calculate the optimal grid size based on the desired number of grids
    float gridSize = maxDimension / desiredGridCount;

    return gridSize;
}