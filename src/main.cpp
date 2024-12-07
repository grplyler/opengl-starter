
// OpenGL Stuff
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// GLM Stuff
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// ImGui Stuff
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <ImGuizmo.h>

// Image Loading
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// My Stuff
#include "camera.h"
#include "shader.h"

// Standard Library
#include <iostream>
#include <fstream>
#include <sstream>

// Forward Declarations
unsigned int LoadShader(std::string vertexPath, std::string fragmentPath);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);

// camera
unsigned int SCR_WIDTH = 800;
unsigned int SCR_HEIGHT = 600;
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f,  3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
bool enableMouseMovement = true;


bool useWindow = true;
int gizmoCount = 1;
float camDistance = 8.f;
static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);
static bool useSnap(false);
static float snap[3] = { 1.f, 1.f, 1.f };

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

// Matrix Setup
// Model Matrix
glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(10.0f, 10.0f, 10.0f));

// View Matrix
glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 1.0f, 0.0f));

// Projection Matrix
glm::mat4 projection = glm::perspective(glm::radians(60.0f), 800.0f / 600.0f, 0.1f, 100.0f);

void TransformEnd()
{
   if (useWindow)
   {
      ImGui::End();
   }
   ImGui::PopStyleColor(1);
}

void EditTransform(float* cameraView, float* cameraProjection, float* matrix)
{
    ImGuiIO& io = ImGui::GetIO();
    float windowWidth = (float)ImGui::GetWindowWidth();
    float windowHeight = (float)ImGui::GetWindowHeight();
    if (!useWindow)
    {
       ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
    }
    else
    {
       ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, windowWidth, windowHeight);
    }
    ImGuizmo::Manipulate(cameraView, cameraProjection, mCurrentGizmoOperation, mCurrentGizmoMode, matrix, NULL, useSnap ? &snap[0] : NULL);
}



int main()
{
    // Initialize GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Set window hints *before* creating the window
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // For Mac compatibility
#endif

    GLFWwindow *window = glfwCreateWindow(800, 600, "OpenGL Starter", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwMakeContextCurrent(window);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");



    // Configure OpenGL
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glViewport(0, 0, 800, 600);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_DEPTH_TEST);

    // Load Shaders
    Shader pointShader("point");
    pointShader.setMat4("model", model);
    pointShader.setMat4("view", view);
    pointShader.setMat4("projection", projection);
    pointShader.setFloat("point_size", 10.0f);

    // Set the sphere center and radius
    glUniform3f(glGetUniformLocation(pointShader.ID, "sphere_center"), 0.0f, 0.0f, 0.0f);
    glUniform1f(glGetUniformLocation(pointShader.ID, "sphere_radius"), 1.0f);

    // Set the light properties
    glUniform3f(glGetUniformLocation(pointShader.ID, "light_direction"), 1.0f, -1.0f, -1.0f); // Example direction
    glUniform3f(glGetUniformLocation(pointShader.ID, "light_color"), 1.0f, 1.0f, 1.0f);       // White light
    glUniform3f(glGetUniformLocation(pointShader.ID, "sphere_color"), 0.8f, 0.2f, 0.2f);       // Red sphere

    

    // Create a triangle
    // (x, y, z, r, g, b)
    // float vertices[] = {
    //     -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
    //     0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
    //     0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f};

    // unsigned int VBO, VAO;
    // glGenVertexArrays(1, &VAO);
    // glGenBuffers(1, &VBO);

    // glBindVertexArray(VAO);

    // glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // // Vertex positions
    // glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    // glEnableVertexAttribArray(0);

    // // Vertex colors
    // glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    // glEnableVertexAttribArray(1);

    // // Unbind the VAO
    // glBindVertexArray(0);

    // Create Points
    unsigned int numberOfPoints = 10000;
    float pointVertices[numberOfPoints * 3];
    for (int i = 0; i < numberOfPoints; i++)
    {
        pointVertices[i * 3] = (rand() % 100) / 100.0f - 0.5f;
        pointVertices[i * 3 + 1] = (rand() % 100) / 100.0f - 0.5f;
        pointVertices[i * 3 + 2] = (rand() % 100) / 100.0f - 0.5f;
    }


    unsigned int pointVBO, pointVAO;
    glGenVertexArrays(1, &pointVAO);
    glGenBuffers(1, &pointVBO);

    // Bind the VAO
    glBindVertexArray(pointVAO);

    // Bind the VBO
    glBindBuffer(GL_ARRAY_BUFFER, pointVBO);

    // Copy the vertices data to the VBO
    glBufferData(GL_ARRAY_BUFFER, sizeof(pointVertices), pointVertices, GL_STATIC_DRAW);

    // Set the vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);

    // Enable the vertex attributes
    glEnableVertexAttribArray(0);

    // Unbind the VAO
    glBindVertexArray(0);
    
    // Variables
    glm::vec4 clearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glm::vec3 sphereColor(0.8f, 0.2f, 0.2f);
    glm::vec3 lightDirection(-1.0f, 0.7f, -1.0f);
    glm::vec3 sphereCenter(0.0f, 0.0f, 0.0f);
    glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
    float sphereRadius = 100.0f;
    float specularStrength = 0.5f;

    // Set the sphere center and radius
    glUniform3f(glGetUniformLocation(pointShader.ID, "sphere_center"), 0.0f, 0.0f, 0.0f);
    glUniform1f(glGetUniformLocation(pointShader.ID, "sphere_radius"), sphereRadius);

    // Set the light properties
    glUniform3f(glGetUniformLocation(pointShader.ID, "light_direction"), -1.0f, 0.7f, -1.0f); // Example direction
    glUniform3f(glGetUniformLocation(pointShader.ID, "light_color"), 1.0f, 1.0f, 1.0f);       // White light
    glUniform3f(glGetUniformLocation(pointShader.ID, "sphere_color"), 0.8f, 0.2f, 0.2f);       // Red sphere


    // uniform vec3 sphere_center;   // Center of the sphere
    // uniform float sphere_radius;  // Radius of the sphere
    // uniform vec3 light_direction; // Direction of the light (normalized)
    // uniform vec3 light_color;     // Color of the light (e.g., white: vec3(1.0, 1.0, 1.0))
    // uniform vec3 sphere_color;    // Base color of the sphere

    // pointShader.setVec3("sphere_color", sphereColor.x, sphereColor.y, sphereColor.z);
    // pointShader.setFloat("sphere_radius", sphereRadius);
    // pointShader.setVec3("light_direction", lightDirection.x, lightDirection.y, lightDirection.z);
    // pointShader.setVec3("sphere_center", sphereCenter.x, sphereCenter.y, sphereCenter.z);
    // pointShader.setVec3("light_color", lightColor.x, lightColor.y, lightColor.z);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;  

        // input
        processInput(window);

        // camera/view transformation
        glm::mat4 view = camera.GetViewMatrix();
        pointShader.setMat4("view", view);

        // Clear the screen
        glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render the Points
        pointShader.use();
        pointShader.setFloat("sphere_radius", sphereRadius);
        pointShader.setVec3("light_direction", lightDirection.x, lightDirection.y, lightDirection.z);
        pointShader.setVec3("light_color", 1.0f, 1.0f, 1.0f); // White light
        pointShader.setVec3("sphere_color", sphereColor.x, sphereColor.y, sphereColor.z);
        pointShader.setVec3("view_position", camera.Position.x, camera.Position.y, camera.Position.z);
        pointShader.setFloat("specular_strength", specularStrength);

        glBindVertexArray(pointVAO);
        glDrawArrays(GL_POINTS, 0, numberOfPoints);

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Create ImGui window
        ImGui::Begin("Controls");
        ImGui::ColorEdit3("Clear Color", (float *)&clearColor);
        ImGui::SliderFloat("Sphere Radius", (float *)&sphereRadius, 0.0f, 20.0f);
        ImGui::ColorEdit3("Sphere Color", (float *)&sphereColor);
        ImGui::SliderFloat3("Light Direction", (float *)&lightDirection, -1.0f, 1.0f);
        ImGui::SliderFloat("Specular Strength", &specularStrength, 0.0f, 20.0f);
    

        // TransformStart(glm::value_ptr(view), glm::value_ptr(projection), glm::value_ptr(model));
        ImGuizmo::SetRect(0, 0, 800, 600);
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::BeginFrame();
        ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), mCurrentGizmoOperation, ImGuizmo::LOCAL, glm::value_ptr(model));
        if (ImGuizmo::IsUsing())
        {
            enableMouseMovement = false;
            std::cout << "Using gizmo" << std::endl;
            // Update the model matrix
            pointShader.setMat4("model", model);
        } else {
            // enableMouseMovement = true;
        }


        // Render ImGui
        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window)
{
    if (ImGui::IsKeyPressed(ImGuiKey_T))
        mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    if (ImGui::IsKeyPressed(ImGuiKey_R))
        mCurrentGizmoOperation = ImGuizmo::ROTATE;
    if (ImGui::IsKeyPressed(ImGuiKey_S)) // r Key
        mCurrentGizmoOperation = ImGuizmo::SCALE;
        return;
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        // Show the mouse cursor
        enableMouseMovement = !enableMouseMovement;

        if (enableMouseMovement)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        else
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }    
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);
}


void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
    glViewport(0, 0, width, height);
}

unsigned int LoadShader(std::string vertexPath, std::string fragmentPath)
{

    // Load the vertex shader
    std::string vertexCode;
    std::ifstream vertexFile;
    vertexFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
        vertexFile.open(vertexPath);
        std::stringstream vertexStream;
        vertexStream << vertexFile.rdbuf();
        vertexFile.close();
        vertexCode = vertexStream.str();
    }
    catch (std::ifstream::failure e)
    {
        std::cerr << "Failed to read vertex shader file" << std::endl;
    }

    // Load the fragment shader
    std::string fragmentCode;
    std::ifstream fragmentFile;
    fragmentFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
        fragmentFile.open(fragmentPath);
        std::stringstream fragmentStream;
        fragmentStream << fragmentFile.rdbuf();
        fragmentFile.close();
        fragmentCode = fragmentStream.str();
    }
    catch (std::ifstream::failure e)
    {
        std::cerr << "Failed to read fragment shader file" << std::endl;
    }

    // Compile the shaders
    const char *vertexSource = vertexCode.c_str();
    const char *fragmentSource = fragmentCode.c_str();

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, nullptr);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Failed to compile vertex shader: " << infoLog << std::endl;
    }

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Failed to compile fragment shader: " << infoLog << std::endl;
    }

    // Link the shaders
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Failed to link shader program: " << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
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

    if (enableMouseMovement)
        camera.ProcessMouseMovement(xoffset, yoffset);
}