///////////////////////////////////////////////////////////////////////////////
// viewmanager.cpp
// ============
// manage the viewing of 3D objects within the viewport
///////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>    
#include <iostream>

// declaration of the global variables and defines
namespace
{
	const int WINDOW_WIDTH = 1000;
	const int WINDOW_HEIGHT = 800;
	const char* g_ViewName = "view";
	const char* g_ProjectionName = "projection";

	Camera* g_pCamera = nullptr;

	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;

	float gDeltaTime = 0.0f;
	float gLastFrame = 0.0f;

	bool bOrthographicProjection = false;
}

/***********************************************************
 *  ViewManager()
 ***********************************************************/
ViewManager::ViewManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_pWindow = NULL;
	g_pCamera = new Camera();
	g_pCamera->Position = glm::vec3(0.0f, 10.0f, 7.0f);      // move camera up and forward
	g_pCamera->Front = glm::normalize(glm::vec3(0.0f, -1.0f, -1.0f)); // look downward toward bowl
	g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
	g_pCamera->Zoom = 80;
}

/***********************************************************
 *  ~ViewManager()
 ***********************************************************/
ViewManager::~ViewManager()
{
	m_pShaderManager = NULL;
	m_pWindow = NULL;
	if (NULL != g_pCamera)
	{
		delete g_pCamera;
		g_pCamera = NULL;
	}
}

/***********************************************************
 *  CreateDisplayWindow()
 ***********************************************************/
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
	GLFWwindow* window = nullptr;

	window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, windowTitle, NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return NULL;
	}
	glfwMakeContextCurrent(window);

	// Uncomment to disable OS mouse cursor
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwSetCursorPosCallback(window, &ViewManager::Mouse_Position_Callback);

	// Add mouse scroll callback for zoom/speed
	glfwSetScrollCallback(window, &ViewManager::Mouse_Scroll_Callback);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_pWindow = window;

	return(window);
}

/***********************************************************
 *  Mouse_Position_Callback()
 ***********************************************************/
void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos)
{
	if (gFirstMouse)
	{
		gLastX = xMousePos;
		gLastY = yMousePos;
		gFirstMouse = false;
	}

	float xOffset = xMousePos - gLastX;
	float yOffset = gLastY - yMousePos; // reversed since y-coordinates go from bottom to top

	gLastX = xMousePos;
	gLastY = yMousePos;

	if (g_pCamera)
		g_pCamera->ProcessMouseMovement(xOffset, yOffset);
}

/***********************************************************
 *  Mouse_Scroll_Callback()
 ***********************************************************/
void ViewManager::Mouse_Scroll_Callback(GLFWwindow* window, double xoffset, double yoffset)
{
	if (g_pCamera)
		g_pCamera->ProcessMouseScroll((float)yoffset);
}

/***********************************************************
 *  ProcessKeyboardEvents()
 ***********************************************************/
void ViewManager::ProcessKeyboardEvents()
{
	if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(m_pWindow, true);
	}

	// Camera WASDQE controls
	if (g_pCamera)
	{
		if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS)
			g_pCamera->ProcessKeyboard(FORWARD, gDeltaTime);
		if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS)
			g_pCamera->ProcessKeyboard(BACKWARD, gDeltaTime);
		if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS)
			g_pCamera->ProcessKeyboard(LEFT, gDeltaTime);
		if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS)
			g_pCamera->ProcessKeyboard(RIGHT, gDeltaTime);
		if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS)
			g_pCamera->ProcessKeyboard(UP, gDeltaTime);
		if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS)
			g_pCamera->ProcessKeyboard(DOWN, gDeltaTime);
	}

	// Perspective/Orthographic toggle
	if (glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS)
		bOrthographicProjection = false;
	if (glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_PRESS)
		bOrthographicProjection = true;

	// Optional: When switching to orthographic, force camera to look straight down -Z
	// Uncomment this block if you want "snap" behavior!
	/*
	if (bOrthographicProjection)
	{
		g_pCamera->Position = glm::vec3(0.0f, 7.0f, 7.0f); // Or wherever you want to place the camera
		g_pCamera->Front = glm::vec3(0.0f, -1.0f, -1.0f);  // Look straight ahead (adjust as needed)
		g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
	}
	*/
}

/***********************************************************
 *  PrepareSceneView()
 ***********************************************************/
void ViewManager::PrepareSceneView()
{
	glm::mat4 view;
	glm::mat4 projection;

	// per-frame timing
	float currentFrame = glfwGetTime();
	gDeltaTime = currentFrame - gLastFrame;
	gLastFrame = currentFrame;

	// process any keyboard events that may be waiting in the event queue
	ProcessKeyboardEvents();

	// get the current view matrix from the camera
	view = g_pCamera->GetViewMatrix();

	// Projection matrix selection
	if (bOrthographicProjection)
	{
		float orthoSize = 10.0f;
		float aspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
		projection = glm::ortho(-orthoSize * aspect, orthoSize * aspect, -orthoSize, orthoSize, 0.1f, 100.0f);
	}
	else
	{
		projection = glm::perspective(glm::radians(g_pCamera->Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
	}

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ViewName, view);
		m_pShaderManager->setMat4Value(g_ProjectionName, projection);
		m_pShaderManager->setVec3Value("viewPosition", g_pCamera->Position);
	}
}
