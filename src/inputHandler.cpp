#include "../include/inputHandler.h"

void processInput(GLFWwindow *window, Camera &camera, InputState &inputState, float deltaTime)
{
    // utility binds
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    // movement binds
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, inputState.shiftHeld ? deltaTime * inputState.speedMult : deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, inputState.shiftHeld ? deltaTime * inputState.speedMult : deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, inputState.shiftHeld ? deltaTime * inputState.speedMult : deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, inputState.shiftHeld ? deltaTime * inputState.speedMult : deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, inputState.shiftHeld ? deltaTime * inputState.speedMult : deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, inputState.shiftHeld ? deltaTime * inputState.speedMult : deltaTime);
    // modifier binds
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        inputState.shiftHeld = true;
    if (inputState.shiftHeld && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE)
        inputState.shiftHeld = false;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        inputState.firstToggle = false;
        if (!inputState.qHeld) {
            inputState.cursorDisabled = !inputState.cursorDisabled;
            if (inputState.cursorDisabled) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            } else {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
            inputState.qHeld = true;
        }
    } else {
        inputState.qHeld = false;
    }
    // tool binds
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        inputState.firstToggle = false;
        if (!inputState.fHeld) {
            inputState.flashlightOn = !inputState.flashlightOn;
            inputState.fHeld = true;
        }
    } else {
        inputState.fHeld = false;
    }
}
