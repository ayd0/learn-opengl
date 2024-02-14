#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "../include/model.h"
#include "../include/shader.h"
#include "../include/camera.h"

#include "../include/inputHandler.h"

#include <iostream>

// callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
// utility functions
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
    Shader asteroidsShader("../shaders/generic/instanced-lighting-maps.vs", "../shaders/generic/instanced-lighting-maps.fs");
    Shader lightCubeShader("../shaders/generic/3.3.light_cube.vs", "../shaders/generic/3.3.light_cube_materials.fs");

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    // load models
    // -----------
    Model planet = Model("../resources/models/planet/planet.obj");
    Model rock = Model("../resources/models/rock/rock.obj");

    // generate a large list of semi-random model transformation matrices
    // ------------------------------------------------------------------

    unsigned int amount = 300000; // Total number of asteroids
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

    // set transformation matrices as an instance vertex attribute (with divisor 1)
    // note: we're cheating a little by taking the, now publicly declared, VAO of the model's mesh(es) and adding new vertexAttribPointers
    // normally you'd want to do this in a more organized fashion, but for learning purposes this will do.
    // -----------------------------------------------------------------------------------------------------------------------------------
    for (unsigned int i = 0; i < rock.meshes.size(); i++)
    {
        unsigned int VAO = rock.meshes[i].VAO;
        glBindVertexArray(VAO);
        // set attribute pointers for matrix (4 times vec4)
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

    // configure light cube buffer
    unsigned int lightCubeVBO, lightCubeVAO;
    glGenVertexArrays(1, &lightCubeVAO);
    glGenBuffers(1, &lightCubeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, lightCubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lightCubeVertices), lightCubeVertices, GL_STATIC_DRAW);
    glBindVertexArray(lightCubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lightCubeVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // camera properties
    camera.Position = glm::vec3(100.0f, 450.0f, camera.Position.y);

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
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // view/projection transformations
        // -------------------------------
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, NEAR_PLANE, FAR_PLANE);
        glm::mat4 view = camera.GetViewMatrix();

        // lighting properties
        // -------------------
        glm::vec3 dirColor = glm::vec3(1.0f);
        glm::vec3 pointLightPos = glm::vec3(cos(glfwGetTime() * 3.0f) * 150.0f, 10.0f, sin(glfwGetTime() * 3.0f) * 150.0f);

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

        // render asteroids
        // ----------------
        asteroidsShader.use();
        asteroidsShader.setVec3("viewPos", camera.Position);
        // direction lighting
        asteroidsShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
        asteroidsShader.setVec3("dirLight.ambient", dirColor * 0.05f);
        asteroidsShader.setVec3("dirLight.diffuse", dirColor * 0.1f);
        planetShader.setVec3("dirLight.specular", dirColor * 0.5f);
        asteroidsShader.setFloat("shininess", 8.0f);
        // point lighting
        setPointLight(asteroidsShader, 0, pointLightPos, glm::vec3(1.0f));
        // transform matrices
        asteroidsShader.setMat4("projection", projection);
        asteroidsShader.setMat4("view", view);
        asteroidsShader.setInt("texture_diffuse1", 0);
        asteroidsShader.setInt("texture_specular1", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, rock.textures_loaded[0].id);
        for (unsigned int i = 0; i < rock.meshes.size(); ++i) {
            glBindVertexArray(rock.meshes[i].VAO); 
            glDrawElementsInstanced(GL_TRIANGLES, static_cast<unsigned int>(rock.meshes[i].indices.size()), GL_UNSIGNED_INT, 0, amount);
            glBindVertexArray(0);
        }

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

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// render point lights
// -------------------
void setPointLight(Shader &shader, int index, glm::vec3 position, glm::vec3 color) { 
    std::string uniform = "pointLights[" + std::to_string(index) + "].";
    shader.setVec3(uniform + "position", position);
    shader.setVec3(uniform + "ambient", color * glm::vec3(0.005f, 0.005f, 0.005f));
    shader.setVec3(uniform + "diffuse", color * glm::vec3(0.8f, 0.8f, 0.8f));
    shader.setVec3(uniform + "specular", color * glm::vec3(1.0f, 1.0f, 1.0f));
    shader.setFloat(uniform + "constant", 0.5f);
    shader.setFloat(uniform + "linear", 0.000009f);
    shader.setFloat(uniform + "quadratic", 0.0000032f);
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
