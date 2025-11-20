#include <GLFW/glfw3.h>
#include <glad/glad.h> // Must be included before GLFW
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <camera.h>
#include <model.h>
#include <primitives.h>
#include <shader.h>
#include <buffers.h>

// ImGui includes
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "imgui/imgui.h"

#include <iostream>
#include <stb_image.h>

#include <vector>
#include <random>
#include <array>
#include <map>
#include <string>

#define DEBUG_MODE

// Initialize static member
std::map<std::string, Shader *> Shader::shaders;

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods);
void processInput(GLFWwindow *window);
// void drawIMGUI(Renderer *myRenderer, iCubeModel *cubeSystem,
//                SceneGraph *sg, std::map<std::string, unsigned int> texMap,
//                treeNode *nodes[]);
// void ShaderEditor(SceneGraph *sg);
void drawIMGUI(GLFWwindow *window, Camera &camera, float &deltaTime,
               float &lastFrame, GBuffer &gbuffer);
unsigned int loadTexture(char const *path);
ImVec4 clear_color = ImVec4(0.01, 0.01, 0.01, 1.00f);

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// camera
Camera camera(glm::vec3(5.0f, 1.0f, 5.0f));
bool firstMouse = true;
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
bool mouseDisabled = false;

// timing
float deltaTime = 0.0f; // time between current frame and last frame
float lastFrame = 0.0f;

int main()
{
  // Initialize GLFW
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  // glfw window creation
  // --------------------
  GLFWwindow *window =
      glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
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
  glfwSetKeyCallback(window, key_callback);

  // Disable V-Sync to unlock framerate
  glfwSwapInterval(0);

  // tell GLFW to capture our mouse
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  mouseDisabled = true;

  // Initialize GLAD (this loads OpenGL function pointers)
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
  {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");

  // configure global opengl state
  // -----------------------------
  glEnable(GL_DEPTH_TEST);
  // // glEnable(GL_CULL_FACE);
  // // glCullFace(GL_BACK);
  // // glFrontFace(GL_CCW);

  // Shader wallShader("data/shaders/backrooms_wall.vs",
  //                   "data/shaders/backrooms_wall.fs", "wallShader");

  // Texture wallTexture;
  // wallTexture.type = "texture_diffuse";
  // wallTexture.path = "data/textures/container.png";
  // wallTexture.id = loadTexture(wallTexture.path.c_str());

  // wallShader.use();
  // wallShader.setInt("material.diffuse", 0);
  // wallShader.setInt("material.specular", 1);

  // // Create maze walls
  // // MazeWalls mazeWalls(1.0f, 2.0f, {wallTexture}, wallShader);

  // // Create a cube
  // RectangularPrism centerCube(1.0f, 1.0f, 1.0f, {wallTexture});

  // Shader modelShader("data/shaders/model.vs", "data/shaders/model.fs",
  //                    "modelShader");

  // Shader modelShader("data/shaders/model.vs", "data/shaders/model.fs", "modelShader");
  Shader modelShader(
      (std::string(RUNTIME_DATA_DIR) + "/shaders/model.vs").c_str(),
      (std::string(RUNTIME_DATA_DIR) + "/shaders/model.fs").c_str(),
      "modelShader");
  // Model myModel("data/models/cow/source/sample-3d_glb.glb");
  Model myModel(std::string(RUNTIME_DATA_DIR) + "/models/cow/source/sample-3d_glb.glb");
  // myModel.setDefaultTexture("data/models/cow/textures/Textured_mesh_1_0.jpeg");
  myModel.setDefaultTexture(std::string(RUNTIME_DATA_DIR) + "/models/cow/textures/Textured_mesh_1_0.jpeg");

  // PBR Plane Model

  // std::cout << "Model loaded with " << myModel.meshes.size() << " meshes"
  //           << std::endl;
  // for (size_t i = 0; i < myModel.meshes.size(); i++)
  // {
  //   std::cout << "Mesh " << i << ": " << myModel.meshes[i].vertices.size()
  //             << " vertices, " << myModel.meshes[i].indices.size() << " indices"
  //             << std::endl;
  // }

  GBuffer gbuffer;
  if (!gbuffer.init(SCR_WIDTH, SCR_HEIGHT))
  {
    std::cout << "GBuffer init failed\n";
  }
  // Shader deferredGeometryShader("data/shaders/deferred.vs", "data/shaders/deferred.fs", "deferredGeometryShader");
  Shader deferredGeometryShader(
      (std::string(RUNTIME_DATA_DIR) + "/shaders/deferred_geometry.vs").c_str(),
      (std::string(RUNTIME_DATA_DIR) + "/shaders/deferred_geometry.fs").c_str(),
      "deferredGeometryShader");
  // Shader deferredLightingShader("data/shaders/fullscreen_quad.vs", "data/shaders/deferred_lighting.fs", "deferredLightingShader");
  Shader deferredLightingShader(
      (std::string(RUNTIME_DATA_DIR) + "/shaders/fullscreen_quad.vs").c_str(),
      (std::string(RUNTIME_DATA_DIR) + "/shaders/deferred_lighting.fs").c_str(),
      "deferredLightingShader");
  Shader HDRShader(
      (std::string(RUNTIME_DATA_DIR) + "/shaders/fullscreen_quad.vs").c_str(),
      (std::string(RUNTIME_DATA_DIR) + "/shaders/hdr.fs").c_str(),
      "HDRShader");

#ifdef DEBUG_MODE
  Shader debugNormalShader(
      (std::string(RUNTIME_DATA_DIR) + "/shaders/fullscreen_quad.vs").c_str(),
      (std::string(RUNTIME_DATA_DIR) + "/shaders/debug/normal_DEBUG.fs").c_str(),
      "debugNormalShader");
  Shader debugDepthShader(
      (std::string(RUNTIME_DATA_DIR) + "/shaders/fullscreen_quad.vs").c_str(),
      (std::string(RUNTIME_DATA_DIR) + "/shaders/debug/depth_DEBUG.fs").c_str(),
      "debugDepthShader");
#endif

  // Fullscreen quad
  unsigned int quadVAO = 0, quadVBO = 0;
  {
    float quadVertices[] = {
        // pos      // uv
        -1.f, -1.f, 0.f, 0.f,
        1.f, -1.f, 1.f, 0.f,
        1.f, 1.f, 1.f, 1.f,
        -1.f, -1.f, 0.f, 0.f,
        1.f, 1.f, 1.f, 1.f,
        -1.f, 1.f, 0.f, 1.f};
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glBindVertexArray(0);
  }

  struct PointLight
  {
    glm::vec3 pos;
    glm::vec3 color;
    float intensity;
    float radius;
    Sphere lightVolume;
  };
  // std::vector<PointLight> pointLights = {
  //     {{2.f, 2.f, 2.f}, {1.f, 0.95f, 0.8f}, 6.f, RectangularPrism{{1.f, 1.f, 1.f}, {2.f, 2.f, 2.f}}},
  //     {{-3.f, 1.5f, -2.f}, {0.6f, 0.8f, 1.f}, 5.f, RectangularPrism{{-4.f, 0.5f, -3.f}, {-2.f, 2.f, -1.f}}}};
  std::vector<PointLight> pointLights = {
      {{2.f, 2.f, 2.f}, {0.f, 0.95f, 0.0f}, 10.f, 10.f, Sphere(0.02f, 36, 18, {})},
      {{-3.f, 1.5f, -2.f}, {1.0f, 0.0f, 0.f}, 6.f, 8.f, Sphere(0.02f, 36, 18, {})}};
  // std::vector<PointLight> pointLights;

  Shader lightVolumeShader(
      (std::string(RUNTIME_DATA_DIR) + "/shaders/lights.vs").c_str(),
      (std::string(RUNTIME_DATA_DIR) + "/shaders/lights.fs").c_str(),
      "lightVolumeShader");

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

    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                                            (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();

    // Geometry pass
    glBindFramebuffer(GL_FRAMEBUFFER, gbuffer.fbo);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    deferredGeometryShader.use();
    deferredGeometryShader.setMat4("projection", projection);
    deferredGeometryShader.setMat4("view", view);
    glm::mat4 modelMat(1.0f);
    deferredGeometryShader.setMat4("model", modelMat);
    myModel.Draw(deferredGeometryShader);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Lighting pass
    /* Inputs:
#version 330 core
out vec4 FragColor;
in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    float radius;
};

uniform int uPointLightCount;
uniform PointLight uPointLights[32];
uniform vec3 viewPos;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoMetal;
uniform sampler2D gRoughAoEmiss;
    */
    // glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    deferredLightingShader.use();
    deferredLightingShader.setVec3("viewPos", camera.Position);
    // Bind GBuffer textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gbuffer.texPosition);
    deferredLightingShader.setInt("gPosition", 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gbuffer.texNormal);
    deferredLightingShader.setInt("gNormal", 1);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gbuffer.texAlbedoSpec);
    deferredLightingShader.setInt("gAlbedoSpec", 2);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gbuffer.texMatProps);
    deferredLightingShader.setInt("gMatProps", 3);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, gbuffer.texEmissive);
    deferredLightingShader.setInt("gEmissive", 4);

    deferredLightingShader.setInt("uPointLightCount", (int)pointLights.size());
    for (size_t i = 0; i < pointLights.size(); i++)
    {
      std::string baseName = "uPointLights[" + std::to_string(i) + "]";
      deferredLightingShader.setVec3(baseName + ".position", pointLights[i].pos);
      deferredLightingShader.setVec3(baseName + ".color", pointLights[i].color);
      deferredLightingShader.setFloat(baseName + ".intensity", pointLights[i].intensity);
      deferredLightingShader.setFloat(baseName + ".radius", pointLights[i].radius);
    }

    // Lighting pass -> HDR FBO
    glBindFramebuffer(GL_FRAMEBUFFER, gbuffer.HDRfbo);
    glClear(GL_COLOR_BUFFER_BIT);
    deferredLightingShader.use();

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Copy depth from GBuffer to default framebuffer
    glBindFramebuffer(GL_READ_FRAMEBUFFER, gbuffer.fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    // glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Tonemap
    HDRShader.use();
    HDRShader.setBool("hdr", true);
    HDRShader.setFloat("exposure", 0.1f);
    HDRShader.setInt("hdrBuffer", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gbuffer.bufHDR);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glEnable(GL_DEPTH_TEST);

#ifdef DEBUG_MODE
    // Debug: render debug visualizations into the debug FBOd
    glBindFramebuffer(GL_FRAMEBUFFER, gbuffer.DEBUGfbo);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Enable both attachments for writing
    GLenum debugBuffers[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, debugBuffers);

    // Render normal debug visualization (writes to attachment 0)
    debugNormalShader.use();
    debugNormalShader.setInt("gNormal", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gbuffer.texNormal);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Render depth debug visualization
    debugDepthShader.use();
    debugDepthShader.setInt("gPosition", 0);
    debugDepthShader.setFloat("nearPlane", 0.1f);
    debugDepthShader.setFloat("farPlane", 100.0f);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gbuffer.texPosition);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif

    // Draw light volumes (for visualization) - AFTER lighting pass, on main framebuffer
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE); // Don't write to depth buffer

    lightVolumeShader.use();
    lightVolumeShader.setMat4("projection", projection);
    lightVolumeShader.setMat4("view", view);
    lightVolumeShader.setFloat("alpha", 1.0f); // Add alpha uniform

    for (auto &pl : pointLights)
    {
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, pl.pos);
      model = glm::scale(model, glm::vec3(pl.radius));
      lightVolumeShader.setMat4("model", model);
      lightVolumeShader.setVec3("lightColor", pl.color);
      pl.lightVolume.Draw(lightVolumeShader);
    }

    glDepthMask(GL_TRUE); // Re-enable depth writing
    glDisable(GL_BLEND);

    drawIMGUI(window, camera, deltaTime, lastFrame, gbuffer);

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved
    // etc.)
    // -------------------------------------------------------------------------------
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // Cleanup ImGui
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  // glfw: terminate, clearing all previously allocated GLFW resources.
  // ------------------------------------------------------------------
  glfwTerminate();
  return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this
// frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
  if (glfwGetInputMode(window, GLFW_CURSOR) != GLFW_CURSOR_DISABLED)
    return;

  // Only handle continuous movement keys (WASD + Space + Shift)
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    camera.ProcessKeyboard(FORWARD, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    camera.ProcessKeyboard(BACKWARD, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    camera.ProcessKeyboard(LEFT, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    camera.ProcessKeyboard(RIGHT, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    camera.ProcessKeyboard(UP, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
    camera.ProcessKeyboard(DOWN, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback
// function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
  // make sure the viewport matches the new window dimensions; note that width
  // and height will be significantly larger than specified on retina displays.
  glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xposIn, double yposIn)
{
  // Only process mouse movement if cursor is captured
  if (glfwGetInputMode(window, GLFW_CURSOR) != GLFW_CURSOR_DISABLED)
    return;

  float xpos = static_cast<float>(xposIn);
  float ypos = static_cast<float>(yposIn);

  if (firstMouse)
  {
    lastX = xpos;
    lastY = ypos;
    firstMouse = false;
  }

  float xoffset = xpos - lastX;
  float yoffset =
      lastY - ypos; // reversed since y-coordinates go from bottom to top
  lastX = xpos;
  lastY = ypos;

  camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
  if (glfwGetInputMode(window, GLFW_CURSOR) != GLFW_CURSOR_DISABLED)
    return;

  camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

// glfw: key callback for handling discrete key events (press/release)
// -------------------------------------------------------------------
void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods)
{
  // Handle Escape key to close window
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
  {
    glfwSetWindowShouldClose(window, true);
  }

  // Handle Alt key to toggle mouse capture
  if (key == GLFW_KEY_LEFT_ALT && action == GLFW_PRESS)
  {
    mouseDisabled = !mouseDisabled;
    if (mouseDisabled)
    {
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
      firstMouse = true; // Reset mouse to avoid camera jump
    }
    else
    {
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
  }
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const *path)
{
  unsigned int textureID;
  glGenTextures(1, &textureID);

  int width, height, nrComponents;
  unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
  if (data)
  {
    GLenum format;
    if (nrComponents == 1)
      format = GL_RED;
    else if (nrComponents == 3)
      format = GL_RGB;
    else if (nrComponents == 4)
      format = GL_RGBA;

    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                 GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
  }
  else
  {
    std::cout << "Texture failed to load at path: " << path << std::endl;
    stbi_image_free(data);
  }

  return textureID;
}

void drawIMGUI(GLFWwindow *window, Camera &camera, float &deltaTime,
               float &lastFrame, GBuffer &gbuffer)
{
  // Start the Dear ImGui frame
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  // // Show a simple window that we create ourselves. We use a Begin/End pair
  // to created a named window.
  // {
  //     ImGui::Begin("Boxes and Lighting"); // Create a window and append into
  //     it.

  //     ImGui::End();
  // }

  // GBuffer Debug Viewer
#ifdef DEBUG_MODE
  {
    ImGui::Begin("GBuffer Debug Viewer");

    static int selectedBuffer = 0;
    const char *bufferNames[] = {"Position", "Normal", "Depth", "Albedo+Spec", "Metal+Rough+AO", "Emissive"};

    ImGui::Combo("Buffer", &selectedBuffer, bufferNames, IM_ARRAYSIZE(bufferNames));

    // Display the selected buffer
    GLuint texID = 0;
    switch (selectedBuffer)
    {
    case 0:
      texID = gbuffer.texPosition;
      break;
    case 1:
      texID = gbuffer.texNormalDEBUG;
      break;
    case 2:
      texID = gbuffer.texDepthDEBUG;
      break;
    case 3:
      texID = gbuffer.texAlbedoSpec;
      break;
    case 4:
      texID = gbuffer.texMatProps;
      break;
    case 5:
      texID = gbuffer.texEmissive;
      break;
    }

    // Calculate display size (maintain aspect ratio)
    float aspectRatio = (float)SCR_WIDTH / (float)SCR_HEIGHT;
    float displayWidth = 512.0f;
    float displayHeight = displayWidth / aspectRatio;

    if (texID != 0)
    {
      ImGui::Image((void *)(intptr_t)texID, ImVec2(displayWidth, displayHeight),
                   ImVec2(0, 1), ImVec2(1, 0)); // Flip V coordinate
    }

    // Show all buffers as thumbnails
    ImGui::Separator();
    ImGui::Text("All Buffers:");

    float thumbSize = 128.0f;
    float thumbHeight = thumbSize / aspectRatio;

    ImGui::BeginGroup();
    ImGui::Text("Position");
    ImGui::Image((void *)(intptr_t)gbuffer.texPosition, ImVec2(thumbSize, thumbHeight),
                 ImVec2(0, 1), ImVec2(1, 0));
    ImGui::EndGroup();

    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Text("Normal");
    ImGui::Image((void *)(intptr_t)gbuffer.texNormalDEBUG, ImVec2(thumbSize, thumbHeight),
                 ImVec2(0, 1), ImVec2(1, 0));
    ImGui::EndGroup();

    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Text("Depth");
    ImGui::Image((void *)(intptr_t)gbuffer.texDepthDEBUG, ImVec2(thumbSize, thumbHeight),
                 ImVec2(0, 1), ImVec2(1, 0));
    ImGui::EndGroup();

    ImGui::BeginGroup();
    ImGui::Text("Albedo+Spec");
    ImGui::Image((void *)(intptr_t)gbuffer.texAlbedoSpec, ImVec2(thumbSize, thumbHeight),
                 ImVec2(0, 1), ImVec2(1, 0));
    ImGui::EndGroup();

    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Text("Metal+Rough+AO");
    ImGui::Image((void *)(intptr_t)gbuffer.texMatProps, ImVec2(thumbSize, thumbHeight),
                 ImVec2(0, 1), ImVec2(1, 0));
    ImGui::EndGroup();

    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Text("Emissive");
    ImGui::Image((void *)(intptr_t)gbuffer.texEmissive, ImVec2(thumbSize, thumbHeight),
                 ImVec2(0, 1), ImVec2(1, 0));
    ImGui::EndGroup();

    ImGui::End();
  }
#endif

  // Shader editor
  {
    ImGui::Begin("Shader Editor"); // Create a window and append into it.
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    static Shader *gShader = nullptr;

    if (ImGui::BeginTabBar("Shaders", tab_bar_flags))
    {
      for (const auto &[key, s] : Shader::shaders)
      {
        if (ImGui::BeginTabItem(s->name))
        {
          gShader = s;
          ImGui::EndTabItem();
        }
      }

      ImGui::EndTabBar();

      if (gShader != nullptr)
      {
        static ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;
        ImGui::Text("Vertex Shader");
        ImGui::SameLine();
        ImGui::Text("%s", gShader->vertexPath);
        ImGui::InputTextMultiline(
            "##VertexShader", gShader->vtext, IM_ARRAYSIZE(gShader->vtext),
            ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 16), flags);

        ImGui::Text("Fragment Shader");
        ImGui::SameLine();
        ImGui::Text("%s", gShader->fragmentPath);
        ImGui::InputTextMultiline(
            "##FragmentShader", gShader->ftext, IM_ARRAYSIZE(gShader->ftext),
            ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 16), flags);

        if (ImGui::Button("Recompile Shaders"))
          gShader->reload();

        ImGui::SameLine();

        if (ImGui::Button("Save Shaders"))
          gShader->saveShaders();
      }
    }
    ImGui::ColorEdit3(
        "clear color",
        (float *)&clear_color); // Edit 3 floats representing a color
    ImGui::End();
  }

  // Rendering
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// void drawIMGUI(Renderer *myRenderer, iCubeModel *cubeSystem,
//                SceneGraph *sg, std::map<std::string, unsigned int> texMap,
//                treeNode *nodes[])
// {
//     // Start the Dear ImGui frame
//     ImGui_ImplOpenGL3_NewFrame();
//     ImGui_ImplGlfw_NewFrame();
//     ImGui::NewFrame();

//     // Show a simple window that we create ourselves. We use a Begin/End pair
//     to created a named window.
//     {
//         ImGui::Begin("Hello, world!"); // Create a window and append into it.

//         ImGui::Text("This is some useful text.");                      //
//         Display some text (you can use a format strings too)
//         ImGui::Checkbox("Demo Window", &myRenderer->show_demo_window); //
//         Edit bools storing our window open/close state
//         ImGui::Checkbox("Another Window", &myRenderer->show_another_window);

//         ImGui::SliderFloat("float", &myRenderer->f, 0.0f, 1.0f); // Edit 1
//         float using a slider from 0.0f to 1.0f ImGui::ColorEdit3("clear
//         color", (float *)&myRenderer->clear_color); // Edit 3 floats
//         representing a color

//         if (ImGui::Button("Button")) // Buttons return true when clicked
//         (most widgets return true when edited/activated)
//             myRenderer->counter++;
//         ImGui::SameLine();
//         ImGui::Text("counter = %d", myRenderer->counter);

//         ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f /
//         ImGui::GetIO().Framerate, ImGui::GetIO().Framerate); ImGui::End();
//     }

//     if (myRenderer->show_another_window)
//     {
//         ImGui::Begin("Another Window", &myRenderer->show_another_window); //
//         Pass a pointer to our bool variable (the window will have a closing
//         button that will clear the bool when clicked) ImGui::Text("Hello from
//         another window!"); if (ImGui::Button("Close Me"))
//             myRenderer->show_another_window = false;
//         ImGui::End();
//     }

//     if (myRenderer->show_demo_window)
//         ImGui::ShowDemoWindow(&myRenderer->show_demo_window);

//     ShaderEditor(sg);

//     // Rendering
//     ImGui::Render();
//     ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
// }

// void ShaderEditor(SceneGraph *sg)
// {
//     // Show a simple window that we create ourselves. We use a Begin/End pair
//     to created a named window.
//     {
//         ImGui::Begin("Shader Editor"); // Create a window and append into it.

//         ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;

//         Shader *gShader = Shader::shaders["base"];

//         if (ImGui::BeginTabBar("Shaders", tab_bar_flags))
//         {
//             for (const auto &[key, s] : Shader::shaders)
//             {
//                 if (ImGui::BeginTabItem(s->name))
//                 {
//                     gShader = s;
//                     ImGui::EndTabItem();
//                 }
//             }

//             ImGui::EndTabBar();

//             {
//                 static ImGuiInputTextFlags flags =
//                 ImGuiInputTextFlags_AllowTabInput; ImGui::Text("Vertex
//                 Shader"); ImGui::SameLine(); ImGui::Text("%s",
//                 std::filesystem::absolute(gShader->vertexPath).u8string().c_str());
//                 ImGui::InputTextMultiline("Vertex Shader", gShader->vtext,
//                 IM_ARRAYSIZE(gShader->vtext), ImVec2(-FLT_MIN,
//                 ImGui::GetTextLineHeight() * 16), flags);

//                 ImGui::Text("Fragment Shader");
//                 ImGui::SameLine();
//                 ImGui::Text("%s",
//                 std::filesystem::absolute(gShader->fragmentPath).u8string().c_str());
//                 ImGui::InputTextMultiline("Fragment Shader", gShader->ftext,
//                 IM_ARRAYSIZE(gShader->ftext), ImVec2(-FLT_MIN,
//                 ImGui::GetTextLineHeight() * 16), flags);

//                 if (ImGui::Button("reCompile Shaders"))
//                     gShader->reload();

//                 ImGui::SameLine();

//                 if (ImGui::Button("Save Shaders"))
//                     gShader->saveShaders();
//             }
//         }

//         if (Material::materials["checkers"] != NULL)
//         {
//             for (int i = 0; i < 2; i++)
//             {
//                 ImGui::PushID(i);

//                 if (ImGui::ImageButton((void *)(intptr_t)texture[i],
//                 ImVec2(64, 64)))
//                     Material::materials["checkers"]->textures[0] =
//                     texture[i];
//                 ImGui::PopID();
//                 ImGui::SameLine();
//             }
//         }

//         ImGui::NewLine();

//         if (Material::materials["checkers"] != NULL)
//             ImGui::SliderFloat("shine5",
//             &Material::materials["checkers"]->shine, 0.0, 1.0);

//         if (Material::materials["shuttle"] != NULL)
//             ImGui::SliderFloat("shine6",
//             &Material::materials["shuttle"]->shine, 0.0, 1.0);

//         ImGui::End();
//     }
// }
