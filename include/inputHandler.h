#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include "shader.h"
#include "camera.h"
#include <GLFW/glfw3.h>
#include "glm/glm.hpp"

struct InputState {
    bool qHeld = false;
    bool fHeld = false;
    bool jHeld = false;
    bool shiftHeld = false;
    float speedMult = 3.0f;
    bool firstToggle = true;
    bool cursorDisabled = true;
    bool flashlightOn = false;
    bool drawDebugLine = false;
};

void processInput(GLFWwindow *window, Camera &camera, InputState &inputState, float deltaTime);

#endif // INPUT_HANDLER_H
