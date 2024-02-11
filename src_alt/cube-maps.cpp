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
unsigned int loadCubemap(vector<std::string> faces);

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
    Shader skyboxShader("../shaders/skybox.vs", "../shaders/skybox.fs");
    Shader reflectionShader("../shaders/skybox-reflect.vs", "../shaders/skybox-reflect.fs");

    // load models
    // -----------
    Model ourModel("../resources/models/backpack/backpack.obj");
    Model lantern("../resources/models/japanese-lamp/JapaneseLamp.obj");
    Model floor("../resources/models/tile-floor/tile-floor.obj");
    Model sphere("../resources/models/tile-ball/tile-ball.obj");

    // set to default
    stbi_set_flip_vertically_on_load(false);

    // draw in wireframe
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // position all pointlights
    glm::vec3 pointLightPositions[] = {
        glm::vec3( 2.0f, 2.0f, -3.0f),
        glm::vec3( 0.0f, 2.0f, 0.0f),
        glm::vec3(-14.0f, 2.0f, -12.0f),
        glm::vec3( 10.0f, 0.0f, -3.0f)
    };  

    // load skybox textures
    // ---------------------
    vector<std::string> skybox_faces
    {
        "../resources/textures/skybox01/right.jpg",
        "../resources/textures/skybox01/left.jpg",
        "../resources/textures/skybox01/top.jpg",
        "../resources/textures/skybox01/bottom.jpg",
        "../resources/textures/skybox01/front.jpg",
        "../resources/textures/skybox01/back.jpg"
    };

    unsigned int skyboxID = loadCubemap(skybox_faces);

    // assign skybox vertex data
    // -------------------------
        float skyboxVertices[] = {
        // positions          
        -500.0f,  500.0f, -500.0f,
        -500.0f, -500.0f, -500.0f,
         500.0f, -500.0f, -500.0f,
         500.0f, -500.0f, -500.0f,
         500.0f,  500.0f, -500.0f,
        -500.0f,  500.0f, -500.0f,

        -500.0f, -500.0f,  500.0f,
        -500.0f, -500.0f, -500.0f,
        -500.0f,  500.0f, -500.0f,
        -500.0f,  500.0f, -500.0f,
        -500.0f,  500.0f,  500.0f,
        -500.0f, -500.0f,  500.0f,

         500.0f, -500.0f, -500.0f,
         500.0f, -500.0f,  500.0f,
         500.0f,  500.0f,  500.0f,
         500.0f,  500.0f,  500.0f,
         500.0f,  500.0f, -500.0f,
         500.0f, -500.0f, -500.0f,

        -500.0f, -500.0f,  500.0f,
        -500.0f,  500.0f,  500.0f,
         500.0f,  500.0f,  500.0f,
         500.0f,  500.0f,  500.0f,
         500.0f, -500.0f,  500.0f,
        -500.0f, -500.0f,  500.0f,

        -500.0f,  500.0f, -500.0f,
         500.0f,  500.0f, -500.0f,
         500.0f,  500.0f,  500.0f,
         500.0f,  500.0f,  500.0f,
        -500.0f,  500.0f,  500.0f,
        -500.0f,  500.0f, -500.0f,

        -500.0f, -500.0f, -500.0f,
        -500.0f, -500.0f,  500.0f,
         500.0f, -500.0f, -500.0f,
         500.0f, -500.0f, -500.0f,
        -500.0f, -500.0f,  500.0f,
         500.0f, -500.0f,  500.0f
    };

    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // shader configuration
    // --------------------
    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);
    reflectionShader.use();
    reflectionShader.setInt("skybox", 0);

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

        // enable shaders before setting uniforms
        mainShader.use();
        
        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
        glm::mat4 view = camera.GetViewMatrix();

        // light properties
        glm::vec3 dirColor = glm::vec3(1.0f, 0.7f, 0.2f);
        glm::vec3 lampColor = glm::vec3(0.9f, 0.68f, 0.08f);
        pointLightPositions[0].x = sin(glfwGetTime()) * 3.5f;
        pointLightPositions[0].z = cos(glfwGetTime()) * 3.5f;

        // floor multiplier (used in lighting position calc)
        const unsigned int floorMult = 200;
        // pointLightPositions[1].z = -abs(cos(glfwGetTime() * .5f) * floorMult * 3);
        pointLightPositions[1] = glm::vec3(0.0f, 5.0f, -10.0f);

        // shader properties
        mainShader.setVec3("viewPos", camera.Position);

        // direction light shadess
        mainShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
        mainShader.setVec3("dirLight.ambient", dirColor * 0.5f);
        mainShader.setVec3("dirLight.diffuse", dirColor * 0.7f);
        mainShader.setVec3("dirLight.specular", dirColor * 0.8f);
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

        // draw reflective sphere
        reflectionShader.use();
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(9.0f, 1.0f, -2.0f));
        model = glm::scale(model, glm::vec3(3.0f));
        reflectionShader.setMat4("model", model);
        reflectionShader.setMat4("view", view);
        reflectionShader.setMat4("projection", projection);
        reflectionShader.setVec3("cameraPos", camera.Position);
        sphere.Draw(reflectionShader);

        // draw skybox
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        skyboxShader.use();
        view = glm::mat4(glm::mat3(camera.GetViewMatrix())); // remove translation from the view matrix
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);
        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxID);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default
                              
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // de-allocate resources
    // ---------------------
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVBO);

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

// loads a cubemap's textures and returns its ID
unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}
