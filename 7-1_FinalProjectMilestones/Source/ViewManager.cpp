///////////////////////////////////////////////////////////////////////////////
// viewmanager.cpp
// ============
// manage the viewing of 3D objects within the viewport - camera, projection
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//  Updated for CS-330 Milestone Three
///////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// declaration of the global variables and defines
namespace
{
    // Variables for window width and height
    const int WINDOW_WIDTH = 1000;
    const int WINDOW_HEIGHT = 800;
    const char* g_ViewName = "view";
    const char* g_ProjectionName = "projection";

    // camera object used for viewing and interacting with the 3D scene
    Camera* g_pCamera = nullptr;

    // mouse movement tracking
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;

    // timing
    float gDeltaTime = 0.0f;
    float gLastFrame = 0.0f;

    // projection toggle
    bool bOrthographicProjection = false;

    // cursor toggle
    bool bCursorDisabled = true;
    bool bTabPressed = false;
}

/***********************************************************
 *  ViewManager()
 ***********************************************************/
ViewManager::ViewManager(ShaderManager* pShaderManager)
{
    m_pShaderManager = pShaderManager;
    m_pWindow = nullptr;

    g_pCamera = new Camera();

    // ===== FIXED CAMERA PARAMETERS =====

    // Pull camera back and up
    g_pCamera->Position = glm::vec3(0.0f, 8.0f, 28.0f);

    // Look slightly downward toward the towers
    g_pCamera->Front = glm::normalize(glm::vec3(0.0f, -0.25f, -1.0f));

    // Standard up vector
    g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);

    // Reasonable FOV for architectural scenes
    g_pCamera->Zoom = 55.0f;

    // Slower movement for precision
    g_pCamera->MovementSpeed = 15.0f;
}


/***********************************************************
 *  ~ViewManager()
 ***********************************************************/
ViewManager::~ViewManager()
{
    m_pShaderManager = nullptr;
    m_pWindow = nullptr;

    if (g_pCamera != nullptr)
    {
        delete g_pCamera;
        g_pCamera = nullptr;
    }
}

/***********************************************************
 *  Mouse_Scroll_Callback()
 *
 *  Adjusts camera movement speed using the scroll wheel
 ***********************************************************/
void Mouse_Scroll_Callback(GLFWwindow* window, double xOffset, double yOffset)
{
    // Increase or decrease movement speed
    g_pCamera->MovementSpeed += static_cast<float>(yOffset) * 2.0f;

    // Clamp movement speed to a safe range
    if (g_pCamera->MovementSpeed < 2.0f)
        g_pCamera->MovementSpeed = 2.0f;

    if (g_pCamera->MovementSpeed > 60.0f)
        g_pCamera->MovementSpeed = 60.0f;
}


/***********************************************************
 *  CreateDisplayWindow()
 ***********************************************************/
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
    GLFWwindow* window = glfwCreateWindow(
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        windowTitle,
        nullptr,
        nullptr);

    if (window == nullptr)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return nullptr;
    }

    glfwMakeContextCurrent(window);

    // Capture mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, &ViewManager::Mouse_Position_Callback);
    glfwSetScrollCallback(window, Mouse_Scroll_Callback);

    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_pWindow = window;
    return window;
}


/***********************************************************
 *  Mouse_Position_Callback()
 ***********************************************************/
void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos)
{
    if (gFirstMouse)
    {
        gLastX = static_cast<float>(xMousePos);
        gLastY = static_cast<float>(yMousePos);
        gFirstMouse = false;
    }

    float xOffset = static_cast<float>(xMousePos) - gLastX;
    float yOffset = gLastY - static_cast<float>(yMousePos);

    gLastX = static_cast<float>(xMousePos);
    gLastY = static_cast<float>(yMousePos);

    g_pCamera->ProcessMouseMovement(xOffset, yOffset);
}



/***********************************************************
 *  ProcessKeyboardEvents()
 ***********************************************************/
void ViewManager::ProcessKeyboardEvents()
{
    // Exit
    if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(m_pWindow, true);

    // Toggle cursor (TAB)
    if (glfwGetKey(m_pWindow, GLFW_KEY_TAB) == GLFW_PRESS)
        bTabPressed = true;

    if (bTabPressed && glfwGetKey(m_pWindow, GLFW_KEY_TAB) == GLFW_RELEASE)
    {
        bTabPressed = false;

        glfwSetInputMode(
            m_pWindow,
            GLFW_CURSOR,
            bCursorDisabled ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);

        bCursorDisabled = !bCursorDisabled;
    }

    // WASD movement
    if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS)
        g_pCamera->ProcessKeyboard(FORWARD, gDeltaTime);

    if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS)
        g_pCamera->ProcessKeyboard(BACKWARD, gDeltaTime);

    if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS)
        g_pCamera->ProcessKeyboard(LEFT, gDeltaTime);

    if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS)
        g_pCamera->ProcessKeyboard(RIGHT, gDeltaTime);

    // Vertical movement (Q / E)
    if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS)
        g_pCamera->Position.y -= g_pCamera->MovementSpeed * gDeltaTime;

    if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS)
        g_pCamera->Position.y += g_pCamera->MovementSpeed * gDeltaTime;

    // Toggle projection (P)
    static bool bPPressed = false;

    if (glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS)
        bPPressed = true;

    if (bPPressed && glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_RELEASE)
    {
        bPPressed = false;
        bOrthographicProjection = !bOrthographicProjection;
    }
}

/***********************************************************
 *  PrepareSceneView()
 ***********************************************************/
void ViewManager::PrepareSceneView()
{
    glm::mat4 view;
    glm::mat4 projection;

    // Timing
    float currentFrame = static_cast<float>(glfwGetTime());
    gDeltaTime = currentFrame - gLastFrame;
    gLastFrame = currentFrame;

    // Input
    ProcessKeyboardEvents();

    // Orthographic camera adjustment
    if (bOrthographicProjection)
    {
        g_pCamera->Position = glm::vec3(0.0f, 20.0f, 0.01f);
        g_pCamera->Front = glm::vec3(0.0f, -1.0f, 0.0f);
        g_pCamera->Up = glm::vec3(0.0f, 0.0f, -1.0f);
    }

    view = g_pCamera->GetViewMatrix();

    // Projection toggle
    if (bOrthographicProjection)
    {
        projection = glm::ortho(
            -20.0f, 20.0f,
            -20.0f, 20.0f,
            0.1f, 100.0f);
    }
    else
    {
        projection = glm::perspective(
            glm::radians(g_pCamera->Zoom),
            static_cast<float>(WINDOW_WIDTH) / WINDOW_HEIGHT,
            0.1f,
            100.0f);
    }

    // Send to shader
    if (m_pShaderManager != nullptr)
    {
        m_pShaderManager->setMat4Value(g_ViewName, view);
        m_pShaderManager->setMat4Value(g_ProjectionName, projection);
        m_pShaderManager->setVec3Value("viewPosition", g_pCamera->Position);
    }
}