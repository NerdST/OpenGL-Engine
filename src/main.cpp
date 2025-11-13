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
#define USE_DEFERRED

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
               float &lastFrame, GBuffer &gbuffer, unsigned int ssaoColor,
               unsigned int ssaoColorBlur);
unsigned int loadTexture(char const *path);
ImVec4 clear_color = ImVec4(0.01, 0.01, 0.01, 1.00f);

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
// const unsigned int SCR_WIDTH = 1920;
// const unsigned int SCR_HEIGHT = 1080;

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

  // std::cout << "Model loaded with " << myModel.meshes.size() << " meshes"
  //           << std::endl;
  // for (size_t i = 0; i < myModel.meshes.size(); i++)
  // {
  //   std::cout << "Mesh " << i << ": " << myModel.meshes[i].vertices.size()
  //             << " vertices, " << myModel.meshes[i].indices.size() << " indices"
  //             << std::endl;
  // }

  GBuffer gbuffer;
#ifdef USE_DEFERRED
  if (!gbuffer.init(SCR_WIDTH, SCR_HEIGHT))
  {
    std::cout << "GBuffer init failed\n";
  }
  // Shader deferredGeometryShader("data/shaders/deferred.vs", "data/shaders/deferred.fs", "deferredGeometryShader");
  Shader deferredGeometryShader(
      (std::string(RUNTIME_DATA_DIR) + "/shaders/deferred.vs").c_str(),
      (std::string(RUNTIME_DATA_DIR) + "/shaders/deferred.fs").c_str(),
      "deferredGeometryShader");
  // Shader deferredLightingShader("data/shaders/fullscreen_quad.vs", "data/shaders/deferred_lighting.fs", "deferredLightingShader");
  Shader deferredLightingShader(
      (std::string(RUNTIME_DATA_DIR) + "/shaders/fullscreen_quad.vs").c_str(),
      (std::string(RUNTIME_DATA_DIR) + "/shaders/deferred_lighting.fs").c_str(),
      "deferredLightingShader");
  // Shader ssaoShader("data/shaders/fullscreen_quad.vs", "data/shaders/ssao.fs", "ssaoShader");
  Shader ssaoShader(
      (std::string(RUNTIME_DATA_DIR) + "/shaders/fullscreen_quad.vs").c_str(),
      (std::string(RUNTIME_DATA_DIR) + "/shaders/ssao.fs").c_str(),
      "ssaoShader");
  // Shader ssaoBlurShader("data/shaders/fullscreen_quad.vs", "data/shaders/ssao_blur.fs", "ssaoBlurShader");
  Shader ssaoBlurShader(
      (std::string(RUNTIME_DATA_DIR) + "/shaders/fullscreen_quad.vs").c_str(),
      (std::string(RUNTIME_DATA_DIR) + "/shaders/ssao_blur.fs").c_str(),
      "ssaoBlurShader");
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

  // SSAO FBOs
  unsigned int ssaoFBO = 0, ssaoBlurFBO = 0;
  unsigned int ssaoColor = 0, ssaoColorBlur = 0;
#ifdef USE_DEFERRED
  glGenFramebuffers(1, &ssaoFBO);
  glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
  glGenTextures(1, &ssaoColor);
  glBindTexture(GL_TEXTURE_2D, ssaoColor);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, SCR_WIDTH, SCR_HEIGHT, 0, GL_RED, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColor, 0);

  glGenFramebuffers(1, &ssaoBlurFBO);
  glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
  glGenTextures(1, &ssaoColorBlur);
  glBindTexture(GL_TEXTURE_2D, ssaoColorBlur);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, SCR_WIDTH, SCR_HEIGHT, 0, GL_RED, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBlur, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif

  // SSAO kernel + noise
  std::array<glm::vec3, 64> ssaoKernel;
  std::vector<glm::vec3> ssaoNoise;
#ifdef USE_DEFERRED
  {
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    std::default_random_engine rng;
    for (int i = 0; i < 64; i++)
    {
      glm::vec3 s(
          dist(rng) * 2.0f - 1.0f,
          dist(rng) * 2.0f - 1.0f,
          dist(rng));
      s = glm::normalize(s);
      float scale = float(i) / 64.0f;
      scale = glm::mix(0.1f, 1.0f, scale * scale);
      ssaoKernel[i] = s * scale;
    }
    for (int i = 0; i < 16; i++)
    {
      ssaoNoise.emplace_back(
          dist(rng) * 2.0f - 1.0f,
          dist(rng) * 2.0f - 1.0f,
          0.0f);
    }
  }
  unsigned int noiseTex = 0;
  {
    glGenTextures(1, &noiseTex);
    glBindTexture(GL_TEXTURE_2D, noiseTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, ssaoNoise.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  }
#endif

  struct PointLight
  {
    glm::vec3 pos;
    glm::vec3 color;
    float radius;
    Sphere lightVolume;
  };
  // std::vector<PointLight> pointLights = {
  //     {{2.f, 2.f, 2.f}, {1.f, 0.95f, 0.8f}, 6.f, RectangularPrism{{1.f, 1.f, 1.f}, {2.f, 2.f, 2.f}}},
  //     {{-3.f, 1.5f, -2.f}, {0.6f, 0.8f, 1.f}, 5.f, RectangularPrism{{-4.f, 0.5f, -3.f}, {-2.f, 2.f, -1.f}}}};
  std::vector<PointLight> pointLights = {
      {{2.f, 2.f, 2.f}, {1.f, 0.95f, 0.8f}, 6.f, Sphere(0.02f, 36, 18, {})},
      {{-3.f, 1.5f, -2.f}, {0.6f, 0.8f, 1.f}, 5.f, Sphere(0.02f, 36, 18, {})}};

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
    //     glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // // be sure to activate shader when setting uniforms/drawing objects
    // wallShader.use();

    // // view/projection transformations
    // glm::mat4 projection =
    //     glm::perspective(glm::radians(camera.Zoom),
    //                      (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    // glm::mat4 view = camera.GetViewMatrix();
    // wallShader.setMat4("projection", projection);
    // wallShader.setMat4("view", view);

    // // world transformation
    // glm::mat4 model = glm::mat4(1.0f);
    // wallShader.setMat4("model", model);

    // wallShader.setVec3("viewPos", camera.Position);
    // wallShader.setVec3("spotLight.position", camera.Position);
    // wallShader.setVec3("spotLight.direction", camera.Front);
    // wallShader.setVec3("spotLight.ambient", 0.5f, 0.5f, 0.5f);
    // wallShader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
    // wallShader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
    // wallShader.setFloat("spotLight.constant", 1.0f);
    // wallShader.setFloat("spotLight.linear", 0.09f);
    // wallShader.setFloat("spotLight.quadratic", 0.032f);
    // wallShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
    // wallShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));

    // wallShader.setFloat("material.shininess", 32.0f);

    // // Bind wall texture
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                                            (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();

#ifdef USE_DEFERRED
    // Geometry pass
    glBindFramebuffer(GL_FRAMEBUFFER, gbuffer.fbo);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    deferredGeometryShader.use();
    deferredGeometryShader.setMat4("projection", projection);
    deferredGeometryShader.setMat4("view", view);
    glm::mat4 modelMat(1.0f);
    deferredGeometryShader.setMat4("model", modelMat);
    deferredGeometryShader.setFloat("uTime", currentFrame);
    myModel.Draw(deferredGeometryShader);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // SSAO pass
    ssaoShader.use();
    ssaoShader.setMat4("projection", projection);
    ssaoShader.setFloat("radius", 0.5f);
    ssaoShader.setFloat("bias", 0.025f);
    for (int i = 0; i < 64; i++)
      ssaoShader.setVec3("samples[" + std::to_string(i) + "]", ssaoKernel[i]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gbuffer.texPosition);
    ssaoShader.setInt("gPosition", 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gbuffer.texNormal);
    ssaoShader.setInt("gNormal", 1);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, noiseTex);
    ssaoShader.setInt("noiseTex", 2);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // SSAO blur
    ssaoBlurShader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ssaoColor);
    ssaoBlurShader.setInt("ssaoInput", 0);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Lighting pass
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
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
    glBindTexture(GL_TEXTURE_2D, gbuffer.texAlbedoMetal);
    deferredLightingShader.setInt("gAlbedoMetal", 2);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gbuffer.texRoughAoEmiss);
    deferredLightingShader.setInt("gRoughAoEmiss", 3);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBlur);
    deferredLightingShader.setInt("ssaoTex", 4);

    deferredLightingShader.setInt("uPointLightCount", (int)pointLights.size());
    for (int i = 0; i < (int)pointLights.size(); ++i)
    {
      deferredLightingShader.setVec3("uPointLights[" + std::to_string(i) + "].position", pointLights[i].pos);
      deferredLightingShader.setVec3("uPointLights[" + std::to_string(i) + "].color", pointLights[i].color);
      deferredLightingShader.setFloat("uPointLights[" + std::to_string(i) + "].radius", pointLights[i].radius);
    }

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Copy depth from GBuffer to default framebuffer
    glBindFramebuffer(GL_READ_FRAMEBUFFER, gbuffer.fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

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
#else
    // Forward fallback (unchanged)
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    modelShader.use();
    modelShader.setMat4("projection", projection);
    modelShader.setMat4("view", view);
    modelShader.setMat4("model", glm::mat4(1.0f));

    //     // Add lighting uniforms for the model (same as wallShader)
    //     modelShader.setVec3("viewPos", camera.Position);
    //     modelShader.setVec3("spotLight.position", camera.Position);
    //     modelShader.setVec3("spotLight.direction", camera.Front);
    //     modelShader.setVec3("spotLight.ambient", 0.2f, 0.2f, 0.2f);
    //     modelShader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
    //     modelShader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
    //     modelShader.setFloat("spotLight.constant", 1.0f);
    //     modelShader.setFloat("spotLight.linear", 0.09f);
    //     modelShader.setFloat("spotLight.quadratic", 0.032f);
    //     modelShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
    //     modelShader.setFloat("spotLight.outerCutOff",
    //                          glm::cos(glm::radians(15.0f)));

    //     // ADD THIS LINE - missing material shininess:
    //     modelShader.setFloat("material.shininess", 32.0f);

    //     model = glm::mat4(1.0f);
    //     model = glm::translate(model, glm::vec3(0.0f, 1.75f, 0.0f));
    //     // model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));
    //     modelShader.setMat4("model", model);
    // #ifdef USE_DEFERRED
    //     glBindFramebuffer(GL_FRAMEBUFFER, gbuffer.fbo);
    //     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //     // geometry pass
    //     deferredGeometryShader.use();
    //     // set matrices, bind PBR textures for each mesh (fallbacks if missing)
    //     myModel.Draw(deferredGeometryShader);
    //     glBindFramebuffer(GL_FRAMEBUFFER, 0);
    //     // SSAO pass then blur (bind gbuffer texPosition/texNormal)
    //     // light pass: bind gbuffer textures + ssao result, draw fullscreen quad
    // #else

    myModel.Draw(modelShader);
#endif

    drawIMGUI(window, camera, deltaTime, lastFrame, gbuffer, ssaoColor, ssaoColorBlur);

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved
    // etc.)
    // -------------------------------------------------------------------------------
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // // build and compile our shader zprogram
  // // ------------------------------------
  // // Shader ourShader("data/shaders/basicCube.vs",
  // "data/shaders/basicCube.fs"); Shader
  // lightingShader("data/shaders/ADSSurface.vs", "data/shaders/ADSSurface.fs",
  // "lighting"); Shader lightCubeShader("data/shaders/glowingSurface.vs",
  // "data/shaders/glowingSurface.fs", "lightCube");

  // // set up vertex data (and buffer(s)) and configure vertex attributes
  // // ------------------------------------------------------------------
  // float vertices[] = {
  //     // positions          // normals           // texture coords
  //     -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
  //     0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,
  //     0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
  //     0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
  //     -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,
  //     -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,

  //     -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
  //     0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
  //     0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
  //     0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
  //     -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
  //     -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,

  //     -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
  //     -0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
  //     -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
  //     -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
  //     -0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
  //     -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,

  //     0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
  //     0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
  //     0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
  //     0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
  //     0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
  //     0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,

  //     -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,
  //     0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,
  //     0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
  //     0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
  //     -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,
  //     -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,

  //     -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
  //     0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
  //     0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
  //     0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
  //     -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
  //     -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f};
  // // positions all containers
  // glm::vec3 cubePositions[] = {
  //     glm::vec3(0.0f, 0.0f, 0.0f),
  //     glm::vec3(2.0f, 5.0f, -15.0f),
  //     glm::vec3(-1.5f, -2.2f, -2.5f),
  //     glm::vec3(-3.8f, -2.0f, -12.3f),
  //     glm::vec3(2.4f, -0.4f, -3.5f),
  //     glm::vec3(-1.7f, 3.0f, -7.5f),
  //     glm::vec3(1.3f, -2.0f, -2.5f),
  //     glm::vec3(1.5f, 2.0f, -2.5f),
  //     glm::vec3(1.5f, 0.2f, -1.5f),
  //     glm::vec3(-1.3f, 1.0f, -1.5f)};
  // // positions of the point lights
  // glm::vec3 pointLightPositions[] = {
  //     glm::vec3(0.7f, 0.2f, 2.0f),
  //     glm::vec3(2.3f, -3.3f, -4.0f),
  //     glm::vec3(-4.0f, 2.0f, -12.0f),
  //     glm::vec3(0.0f, 0.0f, -3.0f)};
  // // first, configure the cube's VAO (and VBO)
  // unsigned int VBO, cubeVAO;
  // glGenVertexArrays(1, &cubeVAO); // create VAO for cube
  // glGenBuffers(1, &VBO);          // create VBO

  // glBindBuffer(GL_ARRAY_BUFFER, VBO);
  // glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // glBindVertexArray(cubeVAO);
  // glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void
  // *)0); glEnableVertexAttribArray(0); glVertexAttribPointer(1, 3, GL_FLOAT,
  // GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
  // glEnableVertexAttribArray(1);
  // glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void
  // *)(6 * sizeof(float))); glEnableVertexAttribArray(2);

  // // second, configure the light's VAO (VBO stays the same; the vertices are
  // the same for the light object which is also a 3D cube) unsigned int
  // lightCubeVAO; glGenVertexArrays(1, &lightCubeVAO);
  // glBindVertexArray(lightCubeVAO);

  // glBindBuffer(GL_ARRAY_BUFFER, VBO);
  // // note that we update the lamp's position attribute's stride to reflect
  // the updated buffer data glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 *
  // sizeof(float), (void *)0); glEnableVertexAttribArray(0);

  // // load textures (we now use a utility function to keep the code more
  // organized)
  // //
  // -----------------------------------------------------------------------------
  // unsigned int diffuseMap = loadTexture("data/textures/container.png");
  // unsigned int specularMap =
  // loadTexture("data/textures/container_specular.png");

  // // shader configuration
  // // --------------------
  // lightingShader.use();
  // lightingShader.setInt("material.diffuse", 0);
  // lightingShader.setInt("material.specular", 1);

  // // render loop
  // // -----------
  // while (!glfwWindowShouldClose(window))
  // {
  //     // per-frame time logic
  //     // --------------------
  //     float currentFrame = static_cast<float>(glfwGetTime());
  //     deltaTime = currentFrame - lastFrame;
  //     lastFrame = currentFrame;

  //     // input
  //     // -----
  //     processInput(window);

  //     // render
  //     // ------
  //     glClearColor(clear_color.x, clear_color.y, clear_color.z,
  //     clear_color.w); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  //     // be sure to activate shader when setting uniforms/drawing objects
  //     lightingShader.use();
  //     lightingShader.setVec3("viewPos", camera.Position);
  //     lightingShader.setFloat("material.shininess", 32.0f);

  //     /*
  //        Here we set all the uniforms for the 5/6 types of lights we have. We
  //        have to set them manually and index the proper PointLight struct in
  //        the array to set each uniform variable. This can be done more
  //        code-friendly by defining light types as classes and set their
  //        values in there, or by using a more efficient uniform approach by
  //        using 'Uniform buffer objects', but that is something we'll discuss
  //        in the 'Advanced GLSL' tutorial.
  //     */
  //     // directional light
  //     lightingShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
  //     lightingShader.setVec3("dirLight.ambient", 0.05f, 0.05f, 0.05f);
  //     lightingShader.setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
  //     lightingShader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);
  //     // point light 1
  //     lightingShader.setVec3("pointLights[0].position",
  //     pointLightPositions[0]);
  //     lightingShader.setVec3("pointLights[0].ambient", 0.05f, 0.05f, 0.05f);
  //     lightingShader.setVec3("pointLights[0].diffuse", 0.8f, 0.8f, 0.8f);
  //     lightingShader.setVec3("pointLights[0].specular", 1.0f, 1.0f, 1.0f);
  //     lightingShader.setFloat("pointLights[0].constant", 1.0f);
  //     lightingShader.setFloat("pointLights[0].linear", 0.09f);
  //     lightingShader.setFloat("pointLights[0].quadratic", 0.032f);
  //     // point light 2
  //     lightingShader.setVec3("pointLights[1].position",
  //     pointLightPositions[1]);
  //     lightingShader.setVec3("pointLights[1].ambient", 0.05f, 0.05f, 0.05f);
  //     lightingShader.setVec3("pointLights[1].diffuse", 0.8f, 0.8f, 0.8f);
  //     lightingShader.setVec3("pointLights[1].specular", 1.0f, 1.0f, 1.0f);
  //     lightingShader.setFloat("pointLights[1].constant", 1.0f);
  //     lightingShader.setFloat("pointLights[1].linear", 0.09f);
  //     lightingShader.setFloat("pointLights[1].quadratic", 0.032f);
  //     // point light 3
  //     lightingShader.setVec3("pointLights[2].position",
  //     pointLightPositions[2]);
  //     lightingShader.setVec3("pointLights[2].ambient", 0.05f, 0.05f, 0.05f);
  //     lightingShader.setVec3("pointLights[2].diffuse", 0.8f, 0.8f, 0.8f);
  //     lightingShader.setVec3("pointLights[2].specular", 1.0f, 1.0f, 1.0f);
  //     lightingShader.setFloat("pointLights[2].constant", 1.0f);
  //     lightingShader.setFloat("pointLights[2].linear", 0.09f);
  //     lightingShader.setFloat("pointLights[2].quadratic", 0.032f);
  //     // point light 4
  //     lightingShader.setVec3("pointLights[3].position",
  //     pointLightPositions[3]);
  //     lightingShader.setVec3("pointLights[3].ambient", 0.05f, 0.05f, 0.05f);
  //     lightingShader.setVec3("pointLights[3].diffuse", 0.8f, 0.8f, 0.8f);
  //     lightingShader.setVec3("pointLights[3].specular", 1.0f, 1.0f, 1.0f);
  //     lightingShader.setFloat("pointLights[3].constant", 1.0f);
  //     lightingShader.setFloat("pointLights[3].linear", 0.09f);
  //     lightingShader.setFloat("pointLights[3].quadratic", 0.032f);
  //     // spotLight
  //     lightingShader.setVec3("spotLight.position", camera.Position);
  //     lightingShader.setVec3("spotLight.direction", camera.Front);
  //     lightingShader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
  //     lightingShader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
  //     lightingShader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
  //     lightingShader.setFloat("spotLight.constant", 1.0f);
  //     lightingShader.setFloat("spotLight.linear", 0.09f);
  //     lightingShader.setFloat("spotLight.quadratic", 0.032f);
  //     lightingShader.setFloat("spotLight.cutOff",
  //     glm::cos(glm::radians(12.5f)));
  //     lightingShader.setFloat("spotLight.outerCutOff",
  //     glm::cos(glm::radians(15.0f)));

  //     // view/projection transformations
  //     glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
  //     (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f); glm::mat4 view =
  //     camera.GetViewMatrix(); lightingShader.setMat4("projection",
  //     projection); lightingShader.setMat4("view", view);

  //     // world transformation
  //     glm::mat4 model = glm::mat4(1.0f);
  //     lightingShader.setMat4("model", model);

  //     // bind diffuse map
  //     glActiveTexture(GL_TEXTURE0);
  //     glBindTexture(GL_TEXTURE_2D, diffuseMap);
  //     // bind specular map
  //     glActiveTexture(GL_TEXTURE1);
  //     glBindTexture(GL_TEXTURE_2D, specularMap);

  //     // render containers
  //     glBindVertexArray(cubeVAO);
  //     for (unsigned int i = 0; i < 10; i++)
  //     {
  //         // calculate the model matrix for each object and pass it to shader
  //         before drawing glm::mat4 model = glm::mat4(1.0f); model =
  //         glm::translate(model, cubePositions[i]); float angle = 20.0f * i;
  //         model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f,
  //         0.3f, 0.5f)); lightingShader.setMat4("model", model);

  //         glDrawArrays(GL_TRIANGLES, 0, 36);
  //     }

  //     // also draw the lamp object(s)
  //     lightCubeShader.use();
  //     lightCubeShader.setMat4("projection", projection);
  //     lightCubeShader.setMat4("view", view);

  //     // we now draw as many light bulbs as we have point lights.
  //     glBindVertexArray(lightCubeVAO);
  //     for (unsigned int i = 0; i < 4; i++)
  //     {
  //         model = glm::mat4(1.0f);
  //         model = glm::translate(model, pointLightPositions[i]);
  //         model = glm::scale(model, glm::vec3(0.2f)); // Make it a smaller
  //         cube lightCubeShader.setMat4("model", model);
  //         glDrawArrays(GL_TRIANGLES, 0, 36);
  //     }

  //     // Render ImGui
  //     drawIMGUI(window, camera, deltaTime, lastFrame);

  //     // glfw: swap buffers and poll IO events (keys pressed/released, mouse
  //     moved etc.)
  //     //
  //     -------------------------------------------------------------------------------
  //     glfwSwapBuffers(window);
  //     glfwPollEvents();
  // }

  // // optional: de-allocate all resources once they've outlived their purpose:
  // // ------------------------------------------------------------------------
  // glDeleteVertexArrays(1, &cubeVAO);
  // glDeleteVertexArrays(1, &lightCubeVAO);
  // glDeleteBuffers(1, &VBO);

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
               float &lastFrame, GBuffer &gbuffer, unsigned int ssaoColor,
               unsigned int ssaoColorBlur)
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
  {
    ImGui::Begin("GBuffer Debug Viewer");

    static int selectedBuffer = 0;
    const char *bufferNames[] = {"Position", "Normal", "Albedo+Metal", "Rough+AO+Emiss", "SSAO", "SSAO Blurred"};

    ImGui::Combo("Buffer", &selectedBuffer, bufferNames, IM_ARRAYSIZE(bufferNames));

    // Display the selected buffer
    GLuint texID = 0;
    switch (selectedBuffer)
    {
    case 0:
      texID = gbuffer.texPosition;
      break;
    case 1:
      texID = gbuffer.texNormal;
      break;
    case 2:
      texID = gbuffer.texAlbedoMetal;
      break;
    case 3:
      texID = gbuffer.texRoughAoEmiss;
      break;
    case 4:
      texID = ssaoColor;
      break;
    case 5:
      texID = ssaoColorBlur;
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
    ImGui::Image((void *)(intptr_t)gbuffer.texNormal, ImVec2(thumbSize, thumbHeight),
                 ImVec2(0, 1), ImVec2(1, 0));
    ImGui::EndGroup();

    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Text("Albedo+Metal");
    ImGui::Image((void *)(intptr_t)gbuffer.texAlbedoMetal, ImVec2(thumbSize, thumbHeight),
                 ImVec2(0, 1), ImVec2(1, 0));
    ImGui::EndGroup();

    ImGui::BeginGroup();
    ImGui::Text("Rough+AO+Emiss");
    ImGui::Image((void *)(intptr_t)gbuffer.texRoughAoEmiss, ImVec2(thumbSize, thumbHeight),
                 ImVec2(0, 1), ImVec2(1, 0));
    ImGui::EndGroup();

    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Text("SSAO");
    ImGui::Image((void *)(intptr_t)ssaoColor, ImVec2(thumbSize, thumbHeight),
                 ImVec2(0, 1), ImVec2(1, 0));
    ImGui::EndGroup();

    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Text("SSAO Blurred");
    ImGui::Image((void *)(intptr_t)ssaoColorBlur, ImVec2(thumbSize, thumbHeight),
                 ImVec2(0, 1), ImVec2(1, 0));
    ImGui::EndGroup();

    ImGui::End();
  }

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
