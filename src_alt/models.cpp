#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "../include/glm/glm.hpp"
#include "../include/glm/gtc/matrix_transform.hpp"
#include "../include/glm/gtc/type_ptr.hpp"

#include "../include/shader.h"
#include "../include/camera.h"
#include "../include/model.h"

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

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

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader mainShader("../shaders/3.3.lighting_maps.vs", "../shaders/3.3.models.fs");

    // load models
    // -----------
    Model ourModel("../resources/models/backpack/backpack.obj");
    Model lantern("../resources/models/japanese-lamp/JapaneseLamp.obj");
    Model floor("../resources/models/tile-floor/tile-floor.obj");
    
    // draw in wireframe
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    
    // position all pointlights
    glm::vec3 pointLightPositions[] = {
        glm::vec3( 2.0f, 2.0f, -3.0f),
        glm::vec3( 0.0f, 2.0f, 0.0f),
        glm::vec3(-14.0f, 2.0f, -12.0f),
        glm::vec3( 10.0f, 0.0f, -3.0f)
    };  

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

        // render
        // ------
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // don't forget to enable shader before setting uniforms
        mainShader.use();

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
        glm::mat4 view = camera.GetViewMatrix();

        // light properties
        // glm::vec3 dirColor = glm::vec3(0.94f, 0.6f, 0.5f);
        glm::vec3 dirColor = glm::vec3(0.00f, 0.0f, 0.0f);
        glm::vec3 lampColor = glm::vec3(0.9f, 0.68f, 0.08f);

        pointLightPositions[0].x = sin(glfwGetTime()) * 3.5f;
        pointLightPositions[0].z = cos(glfwGetTime()) * 3.5f;

        const unsigned int floorMult = 200;
        // pointLightPositions[1].z = -abs(cos(glfwGetTime() * .5f) * floorMult * 3);
        pointLightPositions[1] = glm::vec3(0.0f, 5.0f, -10.0f);

        // shader properties
        mainShader.setVec3("viewPos", camera.Position);

        // direction light shadess
        mainShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
        mainShader.setVec3("dirLight.ambient", dirColor * 0.1f);
        mainShader.setVec3("dirLight.diffuse", dirColor * 0.2f);
        mainShader.setVec3("dirLight.specular", dirColor * 0.5f);
        // point light 1 shader
        mainShader.setVec3("pointLights[0].position", pointLightPositions[0]);
        mainShader.setVec3("pointLights[0].ambient", lampColor * glm::vec3(0.5f));
        mainShader.setVec3("pointLights[0].diffuse", lampColor * glm::vec3(0.8f));
        mainShader.setVec3("pointLights[0].specular", lampColor * glm::vec3(1.0));
        mainShader.setFloat("pointLights[0].constant", 1.0f);
        mainShader.setFloat("pointLights[0].linear", 0.09f);
        mainShader.setFloat("pointLights[0].quadratic", 0.032f);
        // point light 2 shader
        mainShader.setVec3("pointLights[1].position", pointLightPositions[1]);
        mainShader.setVec3("pointLights[1].ambient", 0.05f, 0.05f, 0.05f);
        mainShader.setVec3("pointLights[1].diffuse", 0.8f, 0.8f, 0.8f);
        mainShader.setVec3("pointLights[1].specular", 1.0f, 1.0f, 1.0f);
        mainShader.setFloat("pointLights[1].constant", 1.0f);
        mainShader.setFloat("pointLights[1].linear", 0.09f);
        mainShader.setFloat("pointLights[1].quadratic", 0.032f);
        // point light 3 shader
        mainShader.setVec3("pointLights[2].position", pointLightPositions[2]);
        mainShader.setVec3("pointLights[2].ambient", 0.05f, 0.05f, 0.05f);
        mainShader.setVec3("pointLights[2].diffuse", 0.8f, 0.8f, 0.8f);
        mainShader.setVec3("pointLights[2].specular", 1.0f, 1.0f, 1.0f);
        mainShader.setFloat("pointLights[2].constant", 1.0f);
        mainShader.setFloat("pointLights[2].linear", 0.09f);
        mainShader.setFloat("pointLights[2].quadratic", 0.032f);
        // point light 4 shader
        mainShader.setVec3("pointLights[3].position", pointLightPositions[3]);
        mainShader.setVec3("pointLights[3].ambient", 0.05f, 0.05f, 0.05f);
        mainShader.setVec3("pointLights[3].diffuse", 0.8f, 0.8f, 0.8f);
        mainShader.setVec3("pointLights[3].specular", 1.0f, 1.0f, 1.0f);
        mainShader.setFloat("pointLights[3].constant", 1.0f);
        mainShader.setFloat("pointLights[3].linear", 0.09f);
        mainShader.setFloat("pointLights[3].quadratic", 0.032f);
        // spotlight shader
        mainShader.setVec3("spotlight.position", camera.Position);
        mainShader.setVec3("spotlight.direction", camera.Front);
        mainShader.setFloat("spotlight.cutoff", glm::cos(glm::radians(12.5f)));
        mainShader.setFloat("spotlight.outerCutoff", glm::cos(glm::radians(17.5f)));
        // mainShader.setVec3("spotlight.ambient", 0.1f, 0.1f, 0.1f);
        // mainShader.setVec3("spotlight.diffuse", 0.8f, 0.8f, 0.8f);
        // mainShader.setVec3("spotlight.specular", 1.0f, 1.0f, 1.0f);
        mainShader.setVec3("spotlight.ambient", glm::vec3(0.0));
        mainShader.setVec3("spotlight.diffuse", glm::vec3(0.0));
        mainShader.setVec3("spotlight.specular", glm::vec3(0.0));
        mainShader.setFloat("spotlight.constant", 1.0f);
        mainShader.setFloat("spotlight.linear", 0.09f);
        mainShader.setFloat("spotlight.quadratic", 0.032f);

        // material properties
        mainShader.setFloat("material.shininess", 32.0f);

        // set projection/view
        mainShader.setMat4("projection", projection);
        mainShader.setMat4("view", view);

        // render the loaded model (backpack)
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
        mainShader.setMat4("model", model);
        ourModel.Draw(mainShader);

        mainShader.setVec3("emissiveMult", glm::vec3(1.0));

        // render the loaded model (lantern)
        model = glm::mat4(1.0f);
        float angle = glfwGetTime() * glm::radians(45.0f);
        model = glm::translate(model, pointLightPositions[0]);
        model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
        model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
        mainShader.setMat4("model", model);
        lantern.Draw(mainShader);

        // render lantern to trace floors
        model = glm::mat4(1.0f);
        model = glm::translate(model, pointLightPositions[1]);
        model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
        mainShader.setMat4("model", model);
        lantern.Draw(mainShader);

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
            model = glm::translate(model, floorPos - (glm::vec3((float)i) * glm::vec3(0.0f, 0.0f, floorDims.z)));
            model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
            mainShader.setMat4("model", model);
            floor.Draw(mainShader);
        }
        
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

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
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

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
