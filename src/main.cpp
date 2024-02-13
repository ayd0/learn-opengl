#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_glfw.h"
#include "../imgui/imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "../include/glm/glm.hpp"
#include "../include/glm/gtc/matrix_transform.hpp"
#include "../include/glm/gtc/type_ptr.hpp"

#include "../include/shader.h"
#include "../include/camera.h"
#include "../include/model.h"

#include "../include/inputHandler.h"
#include "../include/utils.h"

#include <iostream>

// shaders hoisted
bool stenBorder = false;

// function temps
void renderLines(
        vector<float> &lineVertices,
        unsigned int LINE_BUFFER_LIM,
        unsigned int lineVBO,
        unsigned int lineVAO,
        bool &lineUpdated,
        Shader &lineShader,
        glm::mat4 &projection,
        glm::mat4 &view,
        glm::mat4 &model);
void updateLineVector(vector<float> &lineVertices, const unsigned int LINE_BUFFER_LIM, glm::vec3 lineBegin, glm::vec3 lineEnd, bool &lineUpdated);
void updateLineState(vector<float> &lineVertices, vector<float> &altLineVertices, unsigned int LINE_BUFFER_LIM, bool &lineUpdated, bool &altLineUpdated);
void handleMouseEvents(GLFWwindow *window, glm::mat4 &projection, glm::mat4 &view); 
void iterateDetectSpheres(glm::mat4 &view, glm::mat4 &projection);
bool testRaySphereIntersect(
        const glm::vec3 &rayOrigin,
        const glm::vec3 &rayDirection,
        const glm::vec3 &sphereCenter,
        float sphereRadius,
        glm::mat4 &view,
        glm::mat4 &projection
        );
void applyStencilBorder(
        glm::mat4 &model,
        glm::mat4 &projection,
        glm::mat4 &view,
        glm::vec3 pos,
        Shader &shader,
        Shader &border,
        Model &shape,
        bool applyBorder=stenBorder
        );
void setPointLight(Shader &shader, int index, glm::vec3 position, glm::vec3 color);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

// type defs
struct Sphere {
    glm::vec3 position;
    glm::vec3 dimensions;
    bool selected = false;
};

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
glm::vec3 ray_nds(1.0f);
glm::vec4 ray_clip(1.0f);
glm::vec4 ray_eye(1.0f);
glm::vec3 ray_world(1.0f);

// shaders
bool stenReplace = false;

// objects
vector<Sphere> sphereList = {
    { glm::vec3( 3.0f, 0.0f, -12.0f ), glm::vec3(1.0f) }, 
    { glm::vec3( -3.0f, 0.0f, -16.0f ), glm::vec3(1.0f) }
};

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
    glEnable(GL_STENCIL_TEST);

    // build and compile shaders
    // -------------------------
    Shader mainShader("../shaders/3.3.lighting_maps.vs", "../shaders/3.3.models.fs");
    Shader borderShader("../shaders/stencil-border.vs", "../shaders/stencil-border.fs");
    Shader alphaShader("../shaders/basic.vs", "../shaders/alpha.fs");
    Shader blendingShader("../shaders/basic.vs", "../shaders/blend.fs");
    Shader lineShader("../shaders/very-basic.vs", "../shaders/cast-line.fs");
    Shader pointShader("../shaders/very-basic.vs", "../shaders/pass-through.gs" ,"../shaders/cast-line.fs");
    
    // shader properties
    // -----------------
    mainShader.use();

    // load textures
    // -------------
    unsigned int grassTexture = loadTexture("../resources/textures/grass.png", true);
    unsigned int windowTexture = loadTexture("../resources/textures/blending_transparent_window.png", true);
    unsigned int windowTextureAlt = loadTexture("../resources/textures/blending_transparent_window_alt.png", true);

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    // load models
    // -----------
    Model backpack("../resources/models/backpack/backpack.obj");
    Model lantern("../resources/models/japanese-lamp/JapaneseLamp.obj");
    Model floor("../resources/models/tile-floor/tile-floor.obj");
    Model sphere("../resources/models/tile-ball/tile-ball.obj");

    // draw in wireframe
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    float transparentVertices[] = {
        // positions         // texture Coords (swapped y coordinates because texture is flipped upside down)
        0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
        0.0f, -0.5f,  0.0f,  0.0f,  1.0f,
        1.0f, -0.5f,  0.0f,  1.0f,  1.0f,

        0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
        1.0f, -0.5f,  0.0f,  1.0f,  1.0f,
        1.0f,  0.5f,  0.0f,  1.0f,  0.0f
    };

    // transparent VAO
    unsigned int transparentVAO, transparentVBO;
    glGenVertexArrays(1, &transparentVAO);
    glGenBuffers(1, &transparentVBO);
    glBindVertexArray(transparentVAO);
    glBindBuffer(GL_ARRAY_BUFFER, transparentVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    vector<float> lineVertices = {};
    vector<float> altLineVertices = {};
    bool lineUpdated = false;
    bool altLineUpdated = false;

    // line VAO
    const unsigned int LINE_BUFFER_LIM = 120;
    unsigned int lineVAO, lineVBO;
    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);
    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, LINE_BUFFER_LIM * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // alt line VAO
    unsigned int altLineVAO, altLineVBO;
    glGenVertexArrays(1, &altLineVAO);
    glGenBuffers(1, &altLineVBO);
    glBindVertexArray(altLineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, altLineVBO);
    glBufferData(GL_ARRAY_BUFFER, LINE_BUFFER_LIM * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    float points[] = {
        -0.5f,  0.5f, // top-left
        0.5f,  0.5f, // top-right
        0.5f, -0.5f, // bottom-right
        -0.5f, -0.5f  // bottom-left
    };

    unsigned int pointsVAO, pointsVBO;
    glGenVertexArrays(1, &pointsVAO);
    glGenBuffers(1, &pointsVBO);
    glBindVertexArray(pointsVAO);
    glBindBuffer(GL_ARRAY_BUFFER, pointsVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    
    // position all vegetation
    // -----------------------
    vector<glm::vec3> vegetationPositions 
    {
        glm::vec3(-1.5f, -2.3f, -0.48f),
            glm::vec3( 1.5f, -2.3f, 0.51f),
            glm::vec3( 0.0f, -2.3f, 0.7f),
            glm::vec3(-0.3f, -2.3f, -2.3f),
            glm::vec3 (0.5f, -2.3f, -0.6f)
    };

    // position all windows
    // ------------------------
    vector<glm::vec3> windowPositions = {
        glm::vec3( 0.0f, 0.0f, -6.0f),
        glm::vec3( -4.0f, 0.0f, -9.0f),
    };  

    // position all pointlights
    // ------------------------
    glm::vec3 pointLightPositions[] = {
        glm::vec3( 2.0f, 2.0f, -3.0f),
        glm::vec3( 0.0f, 2.0f, 0.0f),
    };  

    // ImGui implementation
    // --------------------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // ImGui globals
    float dirColorG[3] = { 1.0f, 1.0f, 1.0f };
    float plc1[3] = { 1.0f, 1.0f, 1.0f };
    float plc2[3] = { 1.0f, 1.0f, 1.0f };

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
        processInput(window, camera, inputState, deltaTime);

        // render
        // ------
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // create ImGui frame
        // ------------------
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, NEAR_PLANE, FAR_PLANE);
        glm::mat4 view = camera.GetViewMatrix();

        // light properties
        // glm::vec3 dirColor = glm::vec3(0.94f, 0.6f, 0.5f);
        glm::vec3 dirColor = glm::vec3(dirColorG[0], dirColorG[1], dirColorG[2]);
        glm::vec3 lampColor = glm::vec3(0.9f, 0.68f, 0.08f);

        pointLightPositions[0].x = sin(glfwGetTime()) * 3.5f;
        pointLightPositions[0].z = cos(glfwGetTime()) * 3.5f;

        const unsigned int floorMult = 20;
        pointLightPositions[1] = glm::vec3(0.0f, 5.0f, -10.0f);

        // enable shader before setting uniforms
        mainShader.use();

        // shader properties
        mainShader.setVec3("viewPos", camera.Position);

        // direction light shader
        mainShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
        mainShader.setVec3("dirLight.ambient", dirColor * 0.1f);
        mainShader.setVec3("dirLight.diffuse", dirColor * 0.2f);
        mainShader.setVec3("dirLight.specular", dirColor * 0.5f);
        // point light shaders
        setPointLight(mainShader, 0, pointLightPositions[0], glm::vec3(plc1[0], plc1[1], plc1[2]));
        setPointLight(mainShader, 1, pointLightPositions[1], glm::vec3(plc2[0], plc2[1], plc2[2]));
        // spotlight shader
        mainShader.setVec3("spotlight.position", camera.Position);
        mainShader.setVec3("spotlight.direction", camera.Front);
        mainShader.setFloat("spotlight.cutoff", glm::cos(glm::radians(12.5f)));
        mainShader.setFloat("spotlight.outerCutoff", glm::cos(glm::radians(17.5f)));
        if (inputState.flashlightOn) {
            mainShader.setVec3("spotlight.ambient", 0.1f, 0.1f, 0.1f);
            mainShader.setVec3("spotlight.diffuse", 0.8f, 0.8f, 0.8f);
            mainShader.setVec3("spotlight.specular", 1.0f, 1.0f, 1.0f);
        } else {
            mainShader.setVec3("spotlight.ambient", glm::vec3(0.0));
            mainShader.setVec3("spotlight.diffuse", glm::vec3(0.0));
            mainShader.setVec3("spotlight.specular", glm::vec3(0.0));
        }
        mainShader.setFloat("spotlight.constant", 1.0f);
        mainShader.setFloat("spotlight.linear", 0.09f);
        mainShader.setFloat("spotlight.quadratic", 0.032f);

        // material properties
        mainShader.setFloat("material.shininess", 32.0f);

        // set projection/view
        mainShader.setMat4("projection", projection);
        mainShader.setMat4("view", view);

        // set stencil mask to not write
        glStencilMask(0x00);

        // render the loaded model (lantern)
        glm::mat4 model = glm::mat4(1.0f);
        float angle = glfwGetTime() * glm::radians(45.0f);
        model = glm::translate(model, pointLightPositions[0]);
        model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
        model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
        mainShader.setMat4("model", model);
        mainShader.setVec3("emissiveMult", glm::vec3(plc1[0], plc1[1], plc1[2]));
        lantern.Draw(mainShader);

        // render lantern to trace floors
        model = glm::mat4(1.0f);
        model = glm::translate(model, pointLightPositions[1]);
        model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
        mainShader.setMat4("model", model);
        mainShader.setVec3("emissiveMult", glm::vec3(plc2[0], plc2[1], plc2[2]));
        lantern.Draw(mainShader);

        // enable face culling for floors
        // ------------------------------
        glEnable(GL_CULL_FACE);
        glFrontFace(GL_CCW);

        // render the loaded model (floor)
        glm::vec3 floorPos = glm::vec3(0.0f, -3.0f, 0.0f);
        glm::vec3 floorDims = floor.Get0MeshDimensions();

        model = glm::mat4(1.0f);
        model = glm::translate(model, floorPos);
        model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
        mainShader.setMat4("model", model);
        floor.Draw(mainShader);

        // render multiple floors
        for (unsigned int i = 0; i < floorMult; ++i) {
            model = glm::mat4(1.0f);
            model = glm::translate(model, floorPos + (glm::vec3(0.0f, 0.0f, -floorDims.z * (float)i)));
            model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
            mainShader.setMat4("model", model);
            floor.Draw(mainShader);
            for (unsigned int j = 1; j < floorMult / 3; ++j) {
                // first lateral
                model = glm::mat4(1.0f);
                model = glm::translate(model, floorPos + (glm::vec3(floorDims.x * (float)j, 0.0f, -floorDims.z * (float)i)));
                model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
                mainShader.setMat4("model", model);
                floor.Draw(mainShader);
                // second lateral
                model = glm::mat4(1.0f);
                model = glm::translate(model, floorPos + (glm::vec3(-floorDims.x * (float)j, 0.0f, -floorDims.z * (float)i)));
                model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
                mainShader.setMat4("model", model);
                floor.Draw(mainShader);
            }
        }

        // cleanup
        glDisable(GL_CULL_FACE);

        // render vegetation
        // -----------------
        alphaShader.use();
        alphaShader.setMat4("projection", projection);
        alphaShader.setMat4("view", view);
        glBindVertexArray(transparentVAO);
        glBindTexture(GL_TEXTURE_2D, grassTexture);
        for (unsigned int i = 0; i < vegetationPositions.size(); ++i) {
            model = glm::mat4(1.0f);
            model = glm::translate(model, vegetationPositions[i]);
            alphaShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // cleanup
        mainShader.use();

        // render backpack
        // ---------------
        model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
        applyStencilBorder(model, projection, view, glm::vec3(0.0f), mainShader, borderShader, backpack);

        // render spheres
        // --------------
        for (unsigned int i = 0; i < sphereList.size(); ++i) {
            model = glm::mat4(1.0f);
            glm::vec3 scaleMod = glm::vec3(1.0f);
            model = glm::scale(model, glm::vec3(1.0f));
            applyStencilBorder(model, projection, view, sphereList[i].position, mainShader, borderShader, sphere, sphereList[i].selected);
        }

        // render lines
        // ------------
        // get length, update line vectors
        if (inputState.drawDebugLine) {
            updateLineState(lineVertices, altLineVertices, LINE_BUFFER_LIM, lineUpdated, altLineUpdated);
        }
        lineShader.use();
        lineShader.setBool("alt", true);
        renderLines(lineVertices, LINE_BUFFER_LIM, lineVBO, lineVAO, lineUpdated, lineShader, projection, view, model);
        lineShader.setBool("alt", false);
        renderLines(altLineVertices, LINE_BUFFER_LIM, altLineVBO, altLineVAO, altLineUpdated, lineShader, projection, view, model);
        // cleanup
        mainShader.use();

        // render points
        // -------------
        pointShader.use();
        pointShader.setMat4("projection", projection);
        pointShader.setMat4("view", view);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 1.0f));
        pointShader.setMat4("model", model);
        glBindVertexArray(pointsVAO);
        glDrawArrays(GL_POINTS, 0, 4);

        // render windows
        // --------------
        // enable blending
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // sort by distance for render ordering 
        std::map<float, glm::vec3> sorted;
        for (unsigned int i = 0; i < windowPositions.size(); ++i) {
            float distance = glm::length(camera.Position - windowPositions[i]);
            sorted[distance] = windowPositions[i];
        }

        // setup shader props
        blendingShader.use();
        blendingShader.setMat4("projection", projection);
        blendingShader.setMat4("view", view);
        glBindVertexArray(transparentVAO);
        glBindTexture(GL_TEXTURE_2D, windowTexture);

        // iterate and render windows
        for (std::map<float, glm::vec3>::reverse_iterator it = sorted.rbegin(); it != sorted.rend(); ++it) {
            model = glm::mat4(1.0f);
            model = glm::translate(model, it->second);
            model = glm::scale(model, glm::vec3(3.0f, 5.0f, 3.0f));
            blendingShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // cleanup
        glDisable(GL_BLEND);
        mainShader.use();

        // render ImGui window
        // -------------------
        ImGui::Begin("Debug Panel");
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        // stencil borders
        ImGui::Checkbox("ESP", &stenBorder);
        ImGui::SameLine();
        ImGui::Checkbox("Border", &stenReplace);
        // directional light
        ImGui::Text("Directional Light");
        ImGui::ColorEdit3("Color", dirColorG);
        // point lights
        ImGui::Text("Point Light");
        ImGui::ColorEdit3("Emissive 1", plc1);
        ImGui::ColorEdit3("Emissive 2", plc2);
        // speed mult
        ImGui::SliderFloat("Speed Mult", &inputState.speedMult, 1.0f, 50.0f);
        if (ImGui::Button("Erase Debug Lines")) {
            lineVertices.clear();
            altLineVertices.clear();
        }
        // mouse coords
        ImGui::Text("Coordinates");
        ImGui::Text("mouse: x: %f, y: %f", lastX, lastY);
        ImGui::Text("ray_nds: x: %f, y: %f, z: %f", ray_nds.x, ray_nds.y, ray_nds.z);
        ImGui::Text("ray_clip: x: %f, y: %f, z: %f", ray_clip.x, ray_clip.y, ray_clip.z);
        ImGui::Text("ray_eye: x: %f, y: %f, z: %f", ray_eye.x, ray_eye.y, ray_eye.z);
        ImGui::Text("ray_world: x: %f, y: %f, z: %f", ray_world.x, ray_world.y, ray_world.z);
        // camera pos
        ImGui::Text("Camera Pos: x: %f, y: %f, z: %f", camera.Position.x, camera.Position.y, camera.Position.z);
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // handle mouse events for sphere detection
        // ----------------------------------------
        handleMouseEvents(window, projection, view);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // cleanup ImGui
    // -------------
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // cleanup buffers
    // ---------------
    glDeleteVertexArrays(1, &transparentVAO); 
    glDeleteBuffers(1, &transparentVBO);
    glDeleteVertexArrays(1, &lineVAO); 
    glDeleteBuffers(1, &lineVBO);
    glDeleteVertexArrays(1, &altLineVAO); 
    glDeleteBuffers(1, &altLineVBO);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

void renderLines(
        vector<float> &lineVertices,
        unsigned int LINE_BUFFER_LIM,
        unsigned int lineVBO,
        unsigned int lineVAO,
        bool &lineUpdated,
        Shader &lineShader,
        glm::mat4 &projection,
        glm::mat4 &view,
        glm::mat4 &model) {
    // check limits and if changes made to buffer
    if (lineUpdated && lineVertices.size() < LINE_BUFFER_LIM) {
        glBindBuffer(GL_ARRAY_BUFFER, lineVBO); 
        glBufferSubData(GL_ARRAY_BUFFER, 0, lineVertices.size() * sizeof(float), lineVertices.data());
        lineUpdated = false;
    }
    // setup shader props
    lineShader.setMat4("projection", projection);
    lineShader.setMat4("view", view);
    glBindVertexArray(lineVAO);

    // iterate and render lines
    model = glm::mat4(1.0f);
    lineShader.setMat4("model", model);
    glDrawArrays(GL_LINES, 0, lineVertices.size() / 3);
}

void updateLineVector(vector<float> &lineVertices, const unsigned int LINE_BUFFER_LIM, glm::vec3 lineBegin, glm::vec3 lineEnd, bool &lineUpdated) {
    if (lineVertices.size() >= LINE_BUFFER_LIM - 6)
        lineVertices.erase(lineVertices.begin(), lineVertices.begin() + 6);
    lineVertices.push_back(lineBegin.x);
    lineVertices.push_back(lineBegin.y);
    lineVertices.push_back(lineBegin.z);
    lineVertices.push_back(lineEnd.x);
    lineVertices.push_back(lineEnd.y);
    lineVertices.push_back(lineEnd.z);
    lineUpdated = true;
}

void updateLineState(vector<float> &lineVertices, vector<float> &altLineVertices, unsigned int LINE_BUFFER_LIM, bool &lineUpdated, bool &altLineUpdated) {
    // determine length
    float depth; 
    int x = SCR_WIDTH / 2;
    int y = SCR_HEIGHT / 2;
    // grab depth value from buffer at (x,y)
    glReadPixels(x, y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
    // convert depth value to view space
    float zNDC = depth * 2.0f - 1.0f; // convert from [0, 1] to [-1, 1]
    float zView = (2.0f * NEAR_PLANE * FAR_PLANE) / (FAR_PLANE + NEAR_PLANE - zNDC * (FAR_PLANE - NEAR_PLANE));

    // set length to depth
    float length = abs(zView);

    // push vertex data to vector
    glm::vec3 lineBegin = camera.Position;
    glm::vec3 lineEnd = lineBegin + camera.Front * length;
    if (length < FAR_PLANE)
        updateLineVector(lineVertices, LINE_BUFFER_LIM, lineBegin, lineEnd, lineUpdated);
    else
        updateLineVector(altLineVertices, LINE_BUFFER_LIM, lineBegin, lineEnd, altLineUpdated);
    inputState.drawDebugLine = false;
}


// hacky implementation of ray cast detection, fix and replace!
// ------------------------------------------------------------
void handleMouseEvents(GLFWwindow *window, glm::mat4 &projection, glm::mat4 &view) {
    if (inputState.cursorDisabled && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        // convert mouse coord to NDC
        float x = (2.0f * lastX) / SCR_WIDTH - 1.0f;
        float y = 1.0f - (2.0f * lastY) / SCR_HEIGHT;
        float z = -1.0f; // forward
        ray_nds = glm::vec3(x, y, z);

        // convert NDC to normal clip coords
        ray_clip = glm::vec4(ray_nds.x, ray_nds.y, ray_nds.z, 1.0f);

        // convert clip coords to eye coords
        ray_eye = glm::inverse(projection) * ray_clip;
        ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0f, 0.0f);

        // convert eye to world coords
        ray_world = glm::vec3(glm::inverse(view) * ray_eye);
        ray_world = glm::normalize(ray_world);

        iterateDetectSpheres(view, projection);
    }
}

// iterate through spheres to check for ray intersects
// ---------------------------------------------------
void iterateDetectSpheres(glm::mat4 &view, glm::mat4 &projection) {
    for (Sphere &iSphere : sphereList) {
        float sphereRadius= glm::length(iSphere.dimensions) / 2.0f;
        if (testRaySphereIntersect(camera.Position, ray_world, iSphere.position, sphereRadius, view, projection))
            iSphere.selected = true;
        else 
            iSphere.selected = false;
    }
}

// test if camera ray interesects sphere
// -------------------------------------
bool testRaySphereIntersect(
        const glm::vec3 &rayOrigin,
        const glm::vec3 &rayDirection,
        const glm::vec3 &sphereCenter,
        float sphereRadius,
        glm::mat4 &view,
        glm::mat4 &projection
        ) {
    glm::vec3 L = sphereCenter - rayOrigin;
    float tca = glm::dot(L, rayDirection);
    // if tca is negative, ray is pointing away from sphere
    if (tca < 0) return false;
    float d2 = glm::dot(L, L) - tca * tca;
    // If d^2 > rad^2, ray doesn't intersect with sphere
    float rad2 = sphereRadius * sphereRadius;
    if (d2 > rad2) return false;

    // else, calculate point of intersection
    float thc = sqrt(rad2 - d2);
    float t0 = tca - thc; // closest intersection
    float t1 = tca + thc; // farthest intersection

    // ensure t0 is the closest positive intersection
    if (t0 > t1) std::swap(t0, t1);
    if (t0 < 0) {
        t0 = t1; // if t0 is negative, use t1
        if (t0 < 0) return false; // both negative, return false
    }

    // determine if object is obscured * TODO: Fix this mess, it doesn't work
    // ----------------------------------------------------------------------
    glm::vec3 intersectionPoint = rayOrigin + t0 * rayDirection;
    glm::vec4 clipCoords = projection * view * glm::vec4(intersectionPoint, 1.0f);

    // perform perspective division and viewport transformation
    glm::vec3 ndc = glm::vec3(clipCoords) / clipCoords.w;
    glm::vec2 screenCoords = {
        (ndc.x + 1.0f) * 0.5f * SCR_WIDTH,
        (1.0f - ndc.y) * 0.5f * SCR_HEIGHT
    };

    float depthAtPixel;
    glReadPixels((int)screenCoords.x, (int)screenCoords.y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depthAtPixel);

    float depthAtMouse;
    glReadPixels(static_cast<int>(lastX), SCR_HEIGHT - static_cast<int>(lastY), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depthAtMouse);

    return depthAtMouse <= depthAtPixel;
}

// stencil border mapping
// ----------------------
void applyStencilBorder(
        glm::mat4 &model,
        glm::mat4 &projection,
        glm::mat4 &view,
        glm::vec3 pos,
        Shader &shader,
        Shader &border,
        Model &shape,
        bool applyBorder
        ) {
    // configure stencil test state
    // ----------------------------
    // replace with 1 if both stencil and depth test succeed
    if (applyBorder) {
        glStencilOp(GL_KEEP, stenReplace ? GL_REPLACE : GL_KEEP, GL_REPLACE);
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilMask(0xFF);
    }

    // render the loaded model (shape)
    model = glm::translate(model, pos);
    shader.setMat4("model", model);
    shape.Draw(shader);

    if (applyBorder) {
        // ESP
        glDisable(GL_DEPTH_TEST);
        // second pass: draw upscaled shape buffer
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        // disable writing to stencil buffer
        glStencilMask(0x00);
        border.use();
        border.setMat4("projection", projection);
        border.setMat4("view", view);
        model = glm::mat4(1.0f);
        model = glm::translate(model, pos);
        model = glm::scale(model, glm::vec3(1.02f, 1.02f, 1.02f));
        border.setMat4("model", model);
        shape.Draw(border);

        // reset stencil params
        glStencilMask(0xFF);
        glStencilFunc(GL_ALWAYS, 1, 0xFF);

        // re-enable depth test, cleanup
        glEnable(GL_DEPTH_TEST);
        shader.use();
    }
}

// render point lights
// -------------------
void setPointLight(Shader &shader, int index, glm::vec3 position, glm::vec3 color) { 
    std::string uniform = "pointLights[" + std::to_string(index) + "].";
    shader.setVec3(uniform + "position", position);
    shader.setVec3(uniform + "ambient", color * glm::vec3(0.05f, 0.05f, 0.05f));
    shader.setVec3(uniform + "diffuse", color * glm::vec3(0.8f, 0.8f, 0.8f));
    shader.setVec3(uniform + "specular", color * glm::vec3(1.0f, 1.0f, 1.0f));
    shader.setFloat(uniform + "constant", 1.0f);
    shader.setFloat(uniform + "linear", 0.09f);
    shader.setFloat(uniform + "quadratic", 0.032f);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
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

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
