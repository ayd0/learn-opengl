#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "../include/model.h"
#include "../include/shader.h"
#include "../include/camera.h"

#include "../include/inputHandler.h"
#include "../include/utils.h"

#include <iostream>

// callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
// utility functions
void renderProofScene(const Shader &shader);
void renderCube();
std::vector<glm::mat4> getOmniViews(glm::mat4 &shadowProjection, glm::vec3 &lightPos);
void setPointLight(Shader &shader, int index, glm::vec3 position, glm::vec3 color);

// settings
unsigned int SCR_WIDTH = 1600;
unsigned int SCR_HEIGHT = 900;
float NEAR_PLANE = 0.1f;
float FAR_PLANE = 1000.0f;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// input
InputState inputState;

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    // set callbacks
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

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader planetShader("../shaders/generic/3.3.lighting_maps.vs", "../shaders/generic/instanced-lighting-maps.fs");
    Shader asteroidsShader("../shaders/generic/instanced-lighting-maps.vs", "../shaders/generic/blinn-phong.fs");
    Shader lightCubeShader("../shaders/generic/3.3.light_cube.vs", "../shaders/generic/3.3.light_cube_materials.fs");
    Shader instancedOmniDepthShader("../shaders/util/instanced-omni-depth.vs", "../shaders/util/instanced-omni-depth.gs", "../shaders/util/instanced-omni-depth.fs");
    Shader instancedOmniShadowShader("../shaders/util/instanced-omni-shadows.vs", "../shaders/util/instanced-omni-shadows.fs");
    Shader omniDepthShader("../shaders/util/omni-sm-depth.vs", "../shaders/util/omni-sm-depth.gs", "../shaders/util/omni-sm-depth.fs");
    Shader omniShadowShader("../shaders/util/omni-shadow-map.vs", "../shaders/util/omni-shadow-map.fs");

    unsigned int woodTexture = loadTexture("../resources/textures/wood-floor/wood-floor.jpg");

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    // load models
    // -----------
    Model planet = Model("../resources/models/planet/planet.obj");
    Model rock = Model("../resources/models/rock/rock.obj");

    // generate a large list of semi-random model transformation matrices
    // ------------------------------------------------------------------

    unsigned int amount = 100000; // Total number of asteroids
    unsigned int rings = 7; // Number of rings
    unsigned int asteroidsPerRing = amount / rings; // Distribute asteroids evenly across the rings

    glm::mat4* modelMatrices;
    modelMatrices = new glm::mat4[amount];
    srand(static_cast<unsigned int>(glfwGetTime())); // initialize random seed

    float baseRadius = 150.0; // Base radius for the innermost ring
    float ringSpacing = 90.0f; // Space between each ring
    float offset = 25.0f; // Random offset for each asteroid within a ring

    for (unsigned int ring = 0; ring < rings; ring++) {
        float currentRadius = baseRadius + ringSpacing * ring; // Calculate radius for the current ring

        for (unsigned int i = 0; i < asteroidsPerRing; i++) {
            unsigned int index = ring * asteroidsPerRing + i; // Calculate the index in the modelMatrices array
            glm::mat4 model = glm::mat4(1.0f);

            // 1. Translation: displace along circle with 'currentRadius' in range [-offset, offset]
            float angle = (float)i / (float)asteroidsPerRing * 360.0f;
            float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
            float x = sin(glm::radians(angle)) * currentRadius + displacement;
            displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
            float y = displacement * 0.4f; // Keep height of asteroid field smaller compared to width of x and z
            displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
            float z = cos(glm::radians(angle)) * currentRadius + displacement;
            model = glm::translate(model, glm::vec3(x, y, z));

            // 2. Scale: Scale between 0.05 and 0.25f
            float scale = static_cast<float>((rand() % 20) / 100.0 + 0.05);
            model = glm::scale(model, glm::vec3(scale));

            // 3. Rotation: add random rotation around a (semi)randomly picked rotation axis vector
            float rotAngle = static_cast<float>((rand() % 360));
            model = glm::rotate(model, glm::radians(rotAngle), glm::vec3(0.4f, 0.6f, 0.8f));

            // 4. Now add to list of matrices
            modelMatrices[index] = model;
        }
    }

    // configure instanced array
    // -------------------------
    unsigned int buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, amount * sizeof(glm::mat4), &modelMatrices[0], GL_STATIC_DRAW);

    for (unsigned int i = 0; i < rock.meshes.size(); i++)
    {
        unsigned int VAO = rock.meshes[i].VAO;
        glBindVertexArray(VAO);
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0);
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4)));
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(2 * sizeof(glm::vec4)));
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(3 * sizeof(glm::vec4)));

        glVertexAttribDivisor(3, 1);
        glVertexAttribDivisor(4, 1);
        glVertexAttribDivisor(5, 1);
        glVertexAttribDivisor(6, 1);

        glBindVertexArray(0);
    }

    // light cube vertex data
    float lightCubeVertices[] = {
        -1.0f, -1.0f, -1.0f, 
        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f,  1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,
        1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
        1.0f, -1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f,  1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f, -1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        1.0f, -1.0f,  1.0f
    };

    unsigned int lightCubeVBO, lightCubeVAO;
    glGenVertexArrays(1, &lightCubeVAO);
    glGenBuffers(1, &lightCubeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, lightCubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lightCubeVertices), lightCubeVertices, GL_STATIC_DRAW);
    glBindVertexArray(lightCubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lightCubeVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    camera.Position = glm::vec3(100.0f, 50.0f, camera.Position.y);

    // FBO depth map
    unsigned int depthCubemap, depthMapFBO;
    glGenTextures(1, &depthCubemap);
    const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT= 1024;
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
    for (unsigned int i = 0; i < 6; ++i)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT,
                     SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &depthMapFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    instancedOmniShadowShader.use();
    instancedOmniShadowShader.setInt("depthMap", 1);

    omniShadowShader.use();
    omniShadowShader.setInt("diffuseTexture", 0);
    omniShadowShader.setInt("depthMap", 1);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window, camera, inputState, deltaTime);

        glClearColor(0.01f, 0.01f, 0.01f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::vec3 dirColor = glm::vec3(1.0f);
        glm::vec3 pointLightPos = glm::vec3(cos(glfwGetTime()) * 150.0f, 10.0f, sin(glfwGetTime()) * 150.0f);
        // glm::vec3 pointLightPos = glm::vec3(150.0f, 10.0f, 50.0f);

        // fill depth cubemap
        // ------------------
        float shadowAspect = (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT;
        float SHADOW_NEAR = 1.0f;
        float SHADOW_FAR = 100.0f;
        glm::mat4 shadowProjection = glm::perspective(glm::radians(90.0f), shadowAspect, SHADOW_NEAR, SHADOW_FAR);
        std::vector<glm::mat4> shadowTransforms = getOmniViews(shadowProjection, pointLightPos);
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        instancedOmniDepthShader.use();
        for (unsigned int i = 0; i < 6; ++i)
            instancedOmniDepthShader.setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
        instancedOmniDepthShader.setFloat("far_plane", SHADOW_FAR);
        instancedOmniDepthShader.setVec3("lightPos", pointLightPos);
        for (unsigned int i = 0; i < rock.meshes.size(); ++i) {
            glBindVertexArray(rock.meshes[i].VAO); 
            glDrawElementsInstanced(GL_TRIANGLES, static_cast<unsigned int>(rock.meshes[i].indices.size()), GL_UNSIGNED_INT, 0, amount);
            glBindVertexArray(0);
        }
        // proof cube
        omniDepthShader.use();
        for (unsigned int i = 0; i < 6; ++i)
            omniDepthShader.setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
        omniDepthShader.setFloat("far_plane", SHADOW_FAR);
        omniDepthShader.setVec3("lightPos", pointLightPos);
        renderProofScene(omniDepthShader);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // render asteroids
        // ----------------
        float aspect = (float)SCR_WIDTH / (float)SCR_HEIGHT;
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), aspect, NEAR_PLANE, FAR_PLANE);
        glm::mat4 view = camera.GetViewMatrix();
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        instancedOmniShadowShader.use();
        instancedOmniShadowShader.setMat4("projection", projection);
        instancedOmniShadowShader.setMat4("view", view);
        instancedOmniShadowShader.setVec3("lightPos", pointLightPos);
        instancedOmniShadowShader.setVec3("viewPos", camera.Position);
        instancedOmniShadowShader.setInt("shadows", true);
        instancedOmniShadowShader.setInt("soft", false);
        instancedOmniShadowShader.setFloat("far_plane", SHADOW_FAR);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, rock.textures_loaded[0].id);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
        for (unsigned int i = 0; i < rock.meshes.size(); ++i) {
            glBindVertexArray(rock.meshes[i].VAO); 
            glDrawElementsInstanced(GL_TRIANGLES, static_cast<unsigned int>(rock.meshes[i].indices.size()), GL_UNSIGNED_INT, 0, amount);
            glBindVertexArray(0);
        }

        // render proof cube
        // -----------------
        omniShadowShader.use();
        omniShadowShader.setMat4("projection", projection);
        omniShadowShader.setMat4("view", view);
        omniShadowShader.setVec3("lightPos", pointLightPos);
        omniShadowShader.setVec3("viewPos", camera.Position);
        omniShadowShader.setInt("shadows", true);
        omniShadowShader.setInt("soft", false);
        omniShadowShader.setFloat("far_plane", SHADOW_FAR);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, woodTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
        renderProofScene(omniShadowShader);

        // render planet
        // -------------
        planetShader.use();
        planetShader.setVec3("viewPos", camera.Position);
        // direction lighting
        planetShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
        planetShader.setVec3("dirLight.ambient", dirColor * 0.0f);
        planetShader.setVec3("dirLight.diffuse", dirColor * 0.1f);
        planetShader.setVec3("dirLight.specular", dirColor * 0.5f);
        planetShader.setFloat("shininess", 16.0f);
        // point lighting
        setPointLight(planetShader, 0, pointLightPos, glm::vec3(1.0f));
        // transform matrices
        planetShader.setMat4("projection", projection);
        planetShader.setMat4("view", view);
        planetShader.setInt("texture_diffuse1", 0);
        planetShader.setInt("texture_specular1", 0);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -3.0f, 0.0f));
        model = glm::scale(model, glm::vec3(4.0f));
        planetShader.setMat4("model", model);
        planet.Draw(planetShader);

        // render light cube
        // -----------------
        lightCubeShader.use();
        lightCubeShader.setMat4("projection", projection);
        lightCubeShader.setMat4("view", view);
        lightCubeShader.setVec3("LSCol", glm::vec3(1.0f));
        glBindVertexArray(lightCubeVAO);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(pointLightPos));
        model = glm::scale(model, glm::vec3(0.2f));
        lightCubeShader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

std::vector<glm::mat4> getOmniViews(glm::mat4 &shadowProjection, glm::vec3 &lightPos) {
        std::vector<glm::mat4> shadowTransforms;
        shadowTransforms.push_back(shadowProjection * 
                glm::lookAt(lightPos, lightPos + glm::vec3( 1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0)));
        shadowTransforms.push_back(shadowProjection * 
                glm::lookAt(lightPos, lightPos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0)));
        shadowTransforms.push_back(shadowProjection * 
                glm::lookAt(lightPos, lightPos + glm::vec3( 0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));
        shadowTransforms.push_back(shadowProjection * 
                glm::lookAt(lightPos, lightPos + glm::vec3( 0.0,-1.0, 0.0), glm::vec3(0.0, 0.0,-1.0)));
        shadowTransforms.push_back(shadowProjection * 
                glm::lookAt(lightPos, lightPos + glm::vec3( 0.0, 0.0, 1.0), glm::vec3(0.0,-1.0, 0.0)));
        shadowTransforms.push_back(shadowProjection * 
                glm::lookAt(lightPos, lightPos + glm::vec3( 0.0, 0.0,-1.0), glm::vec3(0.0,-1.0, 0.0)));

        return shadowTransforms;
}

void renderProofScene(const Shader &shader) {
    // room cube
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(150.0f, 1.0f, 70.0f));
    model = glm::scale(model, glm::vec3(20.0f, 20.0f, 1.0f));
    shader.setMat4("model", model);
    renderCube();
}

unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube() {
    if (cubeVAO == 0) {
        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
            // bottom face
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
             1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
             1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left      
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void setPointLight(Shader &shader, int index, glm::vec3 position, glm::vec3 color) { 
    std::string uniform = "pointLights[" + std::to_string(index) + "].";
    shader.setVec3(uniform + "position", position);
    shader.setVec3(uniform + "ambient", color * glm::vec3(0.005f, 0.005f, 0.005f));
    shader.setVec3(uniform + "diffuse", color * glm::vec3(0.8f, 0.8f, 0.8f));
    shader.setVec3(uniform + "specular", color * glm::vec3(1.0f, 1.0f, 1.0f));
    shader.setFloat(uniform + "constant", 0.5f);
    shader.setFloat(uniform + "linear", 0.00009f);
    shader.setFloat(uniform + "quadratic", 0.000032f);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
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

    if (!inputState.cursorDisabled || inputState.firstToggle)
        camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
