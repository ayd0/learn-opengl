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

void checkMouseCoords(GLFWwindow *window, glm::mat4 &projection, glm::mat4 &view); 
void applyStencilBorder(
    glm::mat4 &model,
    glm::mat4 &projection,
    glm::mat4 &view,
    glm::vec3 pos,
    Shader &shader,
    Shader &border,
    Model &shape
);
void setPointLight(Shader &shader, int index, glm::vec3 position, glm::vec3 color);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
unsigned int loadTexture(char const *path, bool alpha);

// settings
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 900;

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

// shaders
bool stenBorder = false;
bool stenReplace = false;

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
    //
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

    // position all spheres
    // --------------------
    vector<glm::vec3> spherePositions = {
        glm::vec3( 3.0f, 0.0f, -12.0f),
        glm::vec3( -3.0f, 0.0f, -16.0f),
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

        // don't forget to enable shader before setting uniforms
        mainShader.use();

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
        glm::mat4 view = camera.GetViewMatrix();
        
        // handle mouse events
        checkMouseCoords(window, projection, view);

        // light properties
        // glm::vec3 dirColor = glm::vec3(0.94f, 0.6f, 0.5f);
        glm::vec3 dirColor = glm::vec3(dirColorG[0], dirColorG[1], dirColorG[2]);
        glm::vec3 lampColor = glm::vec3(0.9f, 0.68f, 0.08f);

        pointLightPositions[0].x = sin(glfwGetTime()) * 3.5f;
        pointLightPositions[0].z = cos(glfwGetTime()) * 3.5f;

        const unsigned int floorMult = 20;
        pointLightPositions[1] = glm::vec3(0.0f, 5.0f, -10.0f);

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
        glCullFace(GL_BACK);
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
        for (glm::vec3 spherePos : spherePositions) {
            model = glm::mat4(1.0f);
            model = glm::scale(model, glm::vec3(1.0f));
            applyStencilBorder(model, projection, view, spherePos, mainShader, borderShader, sphere);
        }

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

        // render spheres
        // --------------
        for (glm::vec3 spherePos : spherePositions) {
            model = glm::mat4(1.0f);
            model = glm::translate(model, spherePos);
            model = glm::scale(model, glm::vec3(1.0f));
            mainShader.setMat4("model", model);
            sphere.Draw(mainShader);
        }

        // render ImGui window
        // -------------------
        ImGui::Begin("Debug Panel");
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Checkbox("ESP", &stenBorder);
        ImGui::SameLine();
        ImGui::Checkbox("Border", &stenReplace);
        ImGui::Text("Directional Light");
        ImGui::ColorEdit3("Color", dirColorG);
        ImGui::Text("Point Light");
        ImGui::ColorEdit3("Emissive 1", plc1);
        ImGui::ColorEdit3("Emissive 2", plc2);
        ImGui::SliderFloat("Speed Mult", &inputState.speedMult, 1.0f, 50.0f);
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

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

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

void checkMouseCoords(GLFWwindow *window, glm::mat4 &projection, glm::mat4 &view) {
    if (inputState.cursorDisabled && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        cout << "Mouse coords: (" << lastX << ", " << lastY << ")" << endl;
        // convert mouse coord to NDC
        float x = (2.0f * lastX) / SCR_WIDTH - 1.0f;
        float y = 1.0f - (2.0f * lastY) / SCR_HEIGHT;
        float z = -1.0f; // forward
        glm::vec3 ray_nds = glm::vec3(x, y, z);
        cout << "NDC mouse coords: (" << ray_nds.x << ", " << ray_nds.y << ", " << ray_nds.z << ")" << endl;
        // convert NDC to homogenous clip coords
        glm::vec4 ray_clip = glm::vec4(ray_nds.x, ray_nds.y, ray_nds.z, 1.0f);
        // convert clip coords to eye coords
        glm::vec4 ray_eye = glm::inverse(projection) * ray_clip;
        ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0f, 0.0f);
        cout << "mouse eye coords: (" << ray_eye.x << ", " << ray_eye.y << ", " << ray_eye.z << ")" << endl;
        // convert eye to world coords
        glm::vec3 ray_world = glm::vec3(glm::inverse(view) * ray_eye);
        ray_world = glm::normalize(ray_world);
        cout << "mouse world coords: (" << ray_world.x << ", " << ray_world.y << ", " << ray_world.z << ")" << endl;
    }

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
    Model &shape
) {
        // configure stencil test state
        // ----------------------------
        // replace with 1 if both stencil and depth test succeed
        if (stenBorder) {
            glStencilOp(GL_KEEP, stenReplace ? GL_REPLACE : GL_KEEP, GL_REPLACE);
            glStencilFunc(GL_ALWAYS, 1, 0xFF);
            glStencilMask(0xFF);
        }

        // render the loaded model (shape)
        model = glm::translate(model, pos);
        shader.setMat4("model", model);
        shape.Draw(shader);

        if (stenBorder) {
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
