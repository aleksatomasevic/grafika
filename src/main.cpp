#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
unsigned int loadTexture(const char *path);
unsigned int loadTexture(char const * path, bool gammaCorrection);
unsigned int loadCubemap(vector<std::string> faces);
void renderQuad();
void renderCube();


// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
bool blinn = false;
bool blinnKeyPressed = false;
bool hdr = true;
bool hdrKeyPressed = false;
float exposure = 1.0f;


// camera
Camera camera(glm::vec3(0.0f, 0.0f, 5.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 backpackPosition = glm::vec3(0.0f);
    float backpackScale = 1.0f;
    PointLight pointLight;
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);

int main() {
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
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    //    stbi_set_flip_vertically_on_load(true);

    // configure global opengl state
    // -----------------------------

    // GLBLEND
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // TODO 01: Change culling
    //    glEnable(GL_CULL_FACE);
    //    glCullFace(GL_BACK);
    glFrontFace(GL_CW);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;



    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader ourShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader marsShader("resources/shaders/2.model_lighting_mars.vs", "resources/shaders/2.model_lighting_mars.fs");
    Shader ourskyboxShader("resources/shaders/6.1.skybox.vs", "resources/shaders/6.1.skybox.fs");
    Shader shader("resources/shaders/3.2.blending.vs", "resources/shaders/3.2.blending.fs");
    Shader shaderMetal("resources/shaders/metalblending.vs","resources/shaders/metalblending.fs");
    Shader Cubeshader("resources/shaders/4.2.face_culling.vs", "resources/shaders/4.2.face_culling.fs");
    Shader shaderLightHdr("resources/shaders/6.lighting.vs", "resources/shaders/6.lighting.fs");
    Shader hdrShader("resources/shaders/6.hdr.vs", "resources/shaders/6.hdr.fs");
    Shader spaceShip1Shader("resources/shaders/space_ship_1.vs", "resources/shaders/space_ship_1.fs" );
    Shader spaceShip2Shader("resources/shaders/space_ship_1.vs", "resources/shaders/space_ship_1.fs" );
    Shader bombShader("resources/shaders/blendingBomb.vs", "resources/shaders/blendingBomb.fs" );

    // configure floating point framebuffer
    // ------------------------------------
    unsigned int hdrFBO;
    glGenFramebuffers(1, &hdrFBO);
    // create floating point color buffer
    unsigned int colorBuffer;
    glGenTextures(1, &colorBuffer);
    glBindTexture(GL_TEXTURE_2D, colorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // create depth buffer (renderbuffer)
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    // attach buffers
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffer, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // lighting info
    // -------------
    // positions
    std::vector<glm::vec3> lightPositions;
    lightPositions.push_back(glm::vec3( 0.0f,  0.0f, 49.5f)); // back light
    lightPositions.push_back(glm::vec3(-1.4f, -1.9f, 9.0f));
    lightPositions.push_back(glm::vec3( 0.0f, -1.8f, 4.0f));
    lightPositions.push_back(glm::vec3( 0.8f, -1.7f, 6.0f));
    // colors
    std::vector<glm::vec3> lightColors;
    lightColors.push_back(glm::vec3(200.0f, 200.0f, 200.0f));
    lightColors.push_back(glm::vec3(0.1f, 0.0f, 0.0f));
    lightColors.push_back(glm::vec3(0.0f, 0.0f, 0.2f));
    lightColors.push_back(glm::vec3(0.0f, 0.1f, 0.0f));

    // shader configuration
    // --------------------
    shaderLightHdr.use();
    shaderLightHdr.setInt("diffuseTexture", 0);
    hdrShader.use();
    hdrShader.setInt("hdrBuffer", 0);

    float cubeVertices[] = {
            // back face
            -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, // bottom-left
            0.5f, -0.5f, -0.5f,  1.0f, 0.0f, // bottom-right
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-right
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-right
            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, // top-left
            -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, // bottom-left
            // front face
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-left
            0.5f,  0.5f,  0.5f,  1.0f, 1.0f, // top-right
            0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // bottom-right
            0.5f,  0.5f,  0.5f,  1.0f, 1.0f, // top-right
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-left
            -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, // top-left
            // left face
            -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // top-right
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // bottom-left
            -0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-left
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // bottom-left
            -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // top-right
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-right
            // right face
            0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // top-left
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-right
            0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // bottom-right
            0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // bottom-right
            0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-left
            0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // top-left
            // bottom face
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // top-right
            0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // bottom-left
            0.5f, -0.5f, -0.5f,  1.0f, 1.0f, // top-left
            0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // bottom-left
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // top-right
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-right
            // top face
            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, // top-left
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-right
            0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // bottom-right
            0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // bottom-right
            -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, // bottom-left
            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f  // top-left
    };

    glm::vec3 cubePositions[] = {
            glm::vec3( -7.0f, -0.6f, -8.5f),
            glm::vec3( -7.0f,  -0.6f, 8.5f),
            glm::vec3(7.0f, -0.6f, -8.5f),
            glm::vec3(7.0f, -0.6f, 8.5f),
    };

    glm::vec3 LaserPositions[] = {
            glm::vec3(28.0f,5.3f,7.5f),
            glm::vec3(25.0f,5.3f,7.6f),
            glm::vec3(26.0f,6.3f,7.6f),
            glm::vec3(32.0f,7.0f,7.9f),
    };

    // cube VAO
    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);


    float planeVertices[] = {
            // positions          // texture Coords
            5.0f, -0.5f,  5.0f,  2.0f, 0.0f,
            -5.0f, -0.5f,  5.0f,  0.0f, 0.0f,
            -5.0f, -0.5f, -5.0f,  0.0f, 2.0f,

            5.0f, -0.5f,  5.0f,  2.0f, 0.0f,
            -5.0f, -0.5f, -5.0f,  0.0f, 2.0f,
            5.0f, -0.5f, -5.0f,  2.0f, 2.0f
    };

    // plane VAO
    unsigned int planeVAO, planeVBO;
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), &planeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    float transparentVertices[] = {
            // positions         // texture Coords (swapped y coordinates because texture is flipped upside down)
            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            0.0f, -0.5f,  0.0f,  0.0f,  1.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,

            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,
            1.0f,  0.5f,  0.0f,  1.0f,  0.0f
    };

    // transparent VAO
    unsigned int transparentVAO, transparentVBO;
    glGenVertexArrays(1, &transparentVAO);
    glGenBuffers(1, &transparentVBO);
    glBindVertexArray(transparentVAO);
    glBindBuffer(GL_ARRAY_BUFFER, transparentVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    unsigned int floorTexture = loadTexture(FileSystem::getPath("resources/textures/fabric-of-squares.png").c_str());
    unsigned int floorMetalTexture = loadTexture(FileSystem::getPath("resources/textures/metal.png").c_str());
    unsigned int cubeTexture  = loadTexture(FileSystem::getPath("resources/textures/matrix.jpg").c_str());
    unsigned int woodTexture = loadTexture(FileSystem::getPath("resources/textures/wood.png").c_str(), true); // note that we're loading the texture as an SRGB texture
    unsigned int laserTexture  = loadTexture(FileSystem::getPath("resources/textures/green1.jpg").c_str());
    unsigned int bombTexture  = loadTexture(FileSystem::getPath("resources/textures/pngwing.com.png").c_str());



    float skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };

    // skybox VAO
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    //skybox
    vector<std::string> faces
            {
//                    FileSystem::getPath("resources/textures/arctic/arctic_lf.tga"),
//                    FileSystem::getPath("resources/textures/arctic/arctic_rt.tga"),
//                    FileSystem::getPath("resources/textures/arctic/arctic_up.tga"),
//                    FileSystem::getPath("resources/textures/arctic/arctic_bk.tga"),
//                    FileSystem::getPath("resources/textures/arctic/arctic_ft.tga"),
//                    FileSystem::getPath("resources/textures/arctic/arctic_dn.tga")

//                    FileSystem::getPath("resources/textures/Footballfield/posx.jpg"),
//                    FileSystem::getPath("resources/textures/Footballfield/negx.jpg"),
//                    FileSystem::getPath("resources/textures/Footballfield/posy.jpg"),
//                    FileSystem::getPath("resources/textures/Footballfield/negy.jpg"),
//                    FileSystem::getPath("resources/textures/Footballfield/posz.jpg"),
//                    FileSystem::getPath("resources/textures/Footballfield/negz.jpg")

//                    FileSystem::getPath("resources/textures/ulukai/corona_lf.png"),
//                    FileSystem::getPath("resources/textures/ulukai/corona_rt.png"),
//                    FileSystem::getPath("resources/textures/ulukai/corona_up.png"),
//                    FileSystem::getPath("resources/textures/ulukai/corona_dn.png"),
//                    FileSystem::getPath("resources/textures/ulukai/corona_ft.png"),
//                    FileSystem::getPath("resources/textures/ulukai/corona_bk.png")

//                    FileSystem::getPath("resources/textures/xpos.png"),
//                    FileSystem::getPath("resources/textures/xneg.png"),
//                    FileSystem::getPath("resources/textures/ypos.png"),
//                    FileSystem::getPath("resources/textures/yneg.png"),
//                    FileSystem::getPath("resources/textures/zpos.png"),
//                    FileSystem::getPath("resources/textures/zneg.png")

                    FileSystem::getPath("resources/textures/svemir1/skybox_right.png"),
                    FileSystem::getPath("resources/textures/svemir1/skybox_left.png"),
                    FileSystem::getPath("resources/textures/svemir1/skybox_up.png"),
                    FileSystem::getPath("resources/textures/svemir1/skybox_down_rotate.png"),
                    FileSystem::getPath("resources/textures/svemir1/skybox_back.png"),
                    FileSystem::getPath("resources/textures/svemir1/skybox_front.png")

//                    FileSystem::getPath("resources/textures/galaxy/galaxy+X.tga"),
//                    FileSystem::getPath("resources/textures/galaxy/galaxy-X.tga"),
//                    FileSystem::getPath("resources/textures/galaxy/galaxy+Y.tga"),
//                    FileSystem::getPath("resources/textures/galaxy/galaxy-Y.tga"),
//                    FileSystem::getPath("resources/textures/galaxy/galaxy+Z.tga"),
//                    FileSystem::getPath("resources/textures/galaxy/galaxy-Z.tga")
            };
    unsigned int cubemapTexture = loadCubemap(faces);
    ourskyboxShader.use();
    ourskyboxShader.setInt("skybox", 0);

    // load models
    // -----------
    Model ourModel1("resources/objects/svemirski/Intergalactic_Spaceship-(Wavefront).obj");
    ourModel1.SetShaderTextureNamePrefix("material.");


    Model ourModel2("resources/objects/mars/Mars_2K.obj");
    ourModel2.SetShaderTextureNamePrefix("material.");


    Model ourModel3("resources/objects/E-45-Aircraft/E_45_Aircraft_obj.obj");
    ourModel3.SetShaderTextureNamePrefix("material.");

    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(1.0f, 4.0f, 0.0);
    pointLight.ambient = glm::vec3(0.1f, 0.1f, 0.1f);
    pointLight.diffuse = glm::vec3(0.6, 0.6, 0.6);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 1.0f;
    pointLight.linear = 0.09f;
    pointLight.quadratic = 0.032f;



    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    float move_delta = 0.0, x, y, z;
    bool to_rotate = true;
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);


        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // don't forget to enable shader before setting uniforms
        marsShader.use();
        pointLight.position = glm::vec3(65.0f*sin(currentFrame), 6.0f, -20.0f*cos(currentFrame));
        marsShader.setVec3("pointLight.position", pointLight.position);
        marsShader.setVec3("pointLight.ambient", pointLight.ambient);
        marsShader.setVec3("pointLight.diffuse",glm::vec3(200.6, 200.6, 200.6));
        marsShader.setVec3("pointLight.specular", pointLight.specular);
        marsShader.setFloat("pointLight.constant", pointLight.constant);
        marsShader.setFloat("pointLight.linear", pointLight.linear);
        marsShader.setFloat("pointLight.quadratic", pointLight.quadratic);
        marsShader.setVec3("viewPosition", programState->camera.Position);
        marsShader.setFloat("material.shininess", 32.0f);

        spaceShip1Shader.use();
        pointLight.position = glm::vec3(50.7f,-10.21f,20.0f);
        spaceShip1Shader.setVec3("pointLight.position", pointLight.position);
        spaceShip1Shader.setVec3("pointLight.ambient", glm::vec3(0.15, 0.15, 0.15));
        spaceShip1Shader.setVec3("pointLight.diffuse",glm::vec3(50.6, 5.6, 1.6));
        spaceShip1Shader.setVec3("pointLight.specular", pointLight.specular);
        spaceShip1Shader.setFloat("pointLight.constant", pointLight.constant);
        spaceShip1Shader.setFloat("pointLight.linear", pointLight.linear);
        spaceShip1Shader.setFloat("pointLight.quadratic", pointLight.quadratic);
        spaceShip1Shader.setVec3("viewPosition", programState->camera.Position);
        spaceShip1Shader.setFloat("material.shininess", 32.0f);

        spaceShip2Shader.use();
        pointLight.position = glm::vec3(25.77f,-25.9f,-50.9f);
        spaceShip2Shader.setVec3("pointLight.position", pointLight.position);
        spaceShip2Shader.setVec3("pointLight.ambient", glm::vec3(0.1, 0.1, 0.1));
        spaceShip2Shader.setVec3("pointLight.diffuse",glm::vec3(500.6, 5.6, 0.6));
        spaceShip2Shader.setVec3("pointLight.specular", pointLight.specular);
        spaceShip2Shader.setFloat("pointLight.constant", pointLight.constant);
        spaceShip2Shader.setFloat("pointLight.linear", pointLight.linear);
        spaceShip2Shader.setFloat("pointLight.quadratic", pointLight.quadratic);
        spaceShip2Shader.setVec3("viewPosition", programState->camera.Position);
        spaceShip2Shader.setFloat("material.shininess", 32.0f);

        // don't forget to enable shader before setting uniforms
        ourShader.use();
      pointLight.position = glm::vec3(5.0 * cos(currentFrame), 4.0f, 5.0 * sin(currentFrame));
//        pointLight.position = glm::vec3(0.0, 4.0f, 0.0);
        ourShader.setVec3("pointLight.position", pointLight.position);
        ourShader.setVec3("pointLight.ambient", pointLight.ambient);
        ourShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        ourShader.setVec3("pointLight.specular", pointLight.specular);
        ourShader.setFloat("pointLight.constant", pointLight.constant);
        ourShader.setFloat("pointLight.linear", pointLight.linear);
        ourShader.setFloat("pointLight.quadratic", pointLight.quadratic);
        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setFloat("material.shininess", 32.0f);
        ourShader.setInt("blinn", blinn);


        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // 1. render scene into floating point framebuffer
        // -----------------------------------------------
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model,
                               programState->backpackPosition); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(programState->backpackScale));    // it's a bit too big for our scene, so scale it down

        move_delta = glfwGetTime();

        if(move_delta < 10.0){
            y = 0.0 + move_delta;
            z = 0.0 + move_delta;
            model = glm::translate(model, glm::vec3(0.0, y, z));
        } else if (move_delta < 10.5){
            z += move_delta / 50;
            model = glm::translate(model, glm::vec3(0.0, y, z));
            z *= 1.00000005;
        } else if (move_delta < 14.0){
            x += move_delta / 50;
            model = glm::translate(model, glm::vec3(x, y, z));
            model = glm::rotate(model, 1.57f, glm::vec3(0.0,1.0,0.0));
        }


// model = glm::rotate(model, (float)glfwGetTime(), glm::vec3(0.0,0.0,1.0));
        ourShader.setMat4("model", model);
        ourModel1.Draw(ourShader);

        //spaceShip1
        spaceShip1Shader.use();
        spaceShip1Shader.setMat4("projection", projection);
        spaceShip1Shader.setMat4("view", view);
        model = glm::mat4(1.0f);
        model = glm::translate(model,
                               glm::vec3(20.0f,4.0f,8.0f)); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(programState->backpackScale));    // it's a bit too big for our scene, so scale it down
        model = glm::rotate(model, 1.57f, glm::vec3(0.0f,1.0f,0.33f));
        ourShader.setMat4("model", model);
        ourModel1.Draw(ourShader);

        //SpaceShip2
        spaceShip2Shader.use();
        spaceShip2Shader.setMat4("projection", projection);
        spaceShip2Shader.setMat4("view", view);
        model = glm::mat4(1.0f);
        model = glm::translate(model,
                               glm::vec3(35.0f,7.0f,8.0f)); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(programState->backpackScale));    // it's a bit too big for our scene, so scale it down
        model = glm::rotate(model, 1.57f, glm::vec3(0.0f,1.0f,0.0f));
//        model = glm::rotate(model, 1.57f/2, glm::vec3(0.0f,0.0f,1.0f));
        ourShader.setMat4("model", model);
        ourModel3.Draw(spaceShip2Shader);

        //render the loaded model 2
        marsShader.use();
        marsShader.setMat4("projection", projection);
        marsShader.setMat4("view", view);
        model = glm::mat4(1.0f);
        model = glm::translate(model,
                               glm::vec3(35.0f,7.0f,-35.0f)); // translate it down so it's at the center of the scene
//        model = glm::rotate(model, (float)(glfwGetTime()/2), glm::vec3(0.0f,1.0f,0.0f));
        model = glm::scale(model, glm::vec3(2.0f));    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model);
        ourModel2.Draw(marsShader);
        glDisable(GL_CULL_FACE);


        // floor
        model = glm::mat4(1.0f);
        shaderMetal.use();
        shaderMetal.setVec3("light.position",  7.0f, -0.6f, 8.5f);
        shaderMetal.setVec3("viewPos", programState->camera.Position);

        // light properties
        shaderMetal.setVec3("light.ambient", 0.1f, 0.1f, 0.1f);
        shaderMetal.setVec3("light.diffuse", 0.5f, 0.5f, 0.5f);
        shaderMetal.setVec3("light.specular", 1.0f, 1.0f, 1.0f);

        // material properties
        shaderMetal.setVec3("material.specular", 0.5f, 0.5f, 0.5f);
        shaderMetal.setFloat("material.shininess", 32.0f);

        glBindVertexArray(planeVAO);
        glBindTexture(GL_TEXTURE_2D, floorMetalTexture);
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        model = glm::mat4(1.0f);
        model = glm::translate(model,
                               glm::vec3(0.0f,-0.70f,0.0f)); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(2.0f));
        shader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        //transparent wall
        model = glm::mat4(1.0f);
        shader.use();
        glBindVertexArray(planeVAO);
        glBindTexture(GL_TEXTURE_2D, floorTexture);
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        model = glm::mat4(1.0f);
        model = glm::translate(model,
                               glm::vec3(9.5f,0.0f,0.0f)); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(1.0f,2.0f,1.0f));
        model = glm::rotate(model, 1.57f, glm::vec3(0.0f,0.0f,1.0f));
        shader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        model = glm::mat4(1.0f);
        glBindVertexArray(planeVAO);
        glBindTexture(GL_TEXTURE_2D, floorTexture);
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        model = glm::mat4(1.0f);
        model = glm::translate(model,
                               glm::vec3(-9.5f,0.0f,0.0f)); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(1.0f,2.0f,1.0f));
        model = glm::rotate(model, 1.57f, glm::vec3(0.0f,0.0f,1.0f));
        shader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        //bomba
        model = glm::mat4(1.0f);
        bombShader.use();
        glBindVertexArray(transparentVAO);
        glBindTexture(GL_TEXTURE_2D, bombTexture);
        bombShader.setMat4("projection", projection);
        bombShader.setMat4("view", view);
        model = glm::mat4(1.0f);
        model = glm::translate(model,
                               glm::vec3(32.2f,6.5f,8.0f)); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(3.0f,3.0f,6.0f));
        model = glm::rotate(model, 1.57f, glm::vec3(0.0f,0.0f,1.0f));
        bombShader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 6);


        for(int i = 0; i < 4; i++) {
            //kocke
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            model = glm::mat4(1.0f);
            Cubeshader.use();
            Cubeshader.setMat4("view", view);
            Cubeshader.setMat4("projection", projection);
            glBindVertexArray(cubeVAO);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, cubeTexture);
            model = glm::translate(model, cubePositions[i]);
            model = glm::scale(model, glm::vec3(2.0f, 2.0f, 2.0f));
            Cubeshader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glDisable(GL_CULL_FACE);
        }

        //laseri
        for(int i = 0; i < 4; i++) {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            model = glm::mat4(1.0f);
            Cubeshader.use();
            Cubeshader.setMat4("view", view);
            Cubeshader.setMat4("projection", projection);
            glBindVertexArray(cubeVAO);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, laserTexture);
            model = glm::translate(model, LaserPositions[i]);
            model = glm::rotate(model, 1.57f / 4, glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, glm::vec3(2.0f, 0.18f, 0.18f));
            shader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glDisable(GL_CULL_FACE);
        }


        shaderLightHdr.use();
        shaderLightHdr.setMat4("projection", projection);
        shaderLightHdr.setMat4("view", view);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, woodTexture);
//       set lighting uniforms
        for (unsigned int i = 0; i < lightPositions.size(); i++)
        {
            shaderLightHdr.setVec3("lights[" + std::to_string(i) + "].Position", lightPositions[i]);
            shaderLightHdr.setVec3("lights[" + std::to_string(i) + "].Color", lightColors[i]);
        }
        shaderLightHdr.setVec3("viewPos", programState->camera.Position);
//      render tunnel
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 25.0));
        model = glm::scale(model, glm::vec3(2.5f, 2.5f, 27.5f));
        shaderLightHdr.setMat4("model", model);
        shaderLightHdr.setInt("inverse_normals", true);
//      renderCube();
        // draw skybox as last
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        ourskyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix())); // remove translation from the view matrix
        ourskyboxShader.setMat4("view", view);
        ourskyboxShader.setMat4("projection", projection);
        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 2. now render floating point color buffer to 2D quad and tonemap HDR colors to default framebuffer's (clamped) color range
        // --------------------------------------------------------------------------------------------------------------------------
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        hdrShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffer);
        hdrShader.setInt("hdr", hdr);
        hdrShader.setFloat("exposure", exposure);
        renderQuad();

//        std::cout << "hdr: " << (hdr ? "on" : "off") << "| exposure: " << exposure << std::endl;

        if (programState->ImGuiEnabled)
            DrawImGui(programState);

//        std::cout << (blinn ? "Blinn-Phong" : "Phong") << std::endl;

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVBO);

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && !blinnKeyPressed)
    {
        blinn = !blinn;
        blinnKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_RELEASE)
    {
        blinnKeyPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !hdrKeyPressed)
    {
        hdr = !hdr;
        hdrKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
    {
        hdrKeyPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    {
        if (exposure > 0.0f)
            exposure -= 0.1f;
        else
            exposure = 0.0f;
    }
    else if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    {
        exposure += 0.1f;
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);
        ImGui::DragFloat3("Backpack position", (float*)&programState->backpackPosition);
        ImGui::DragFloat("Backpack scale", &programState->backpackScale, 0.05, 0.1, 4.0);

        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}

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

unsigned int loadTexture(char const * path)
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
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
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

unsigned int loadTexture(char const * path, bool gammaCorrection)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum internalFormat;
        GLenum dataFormat;
        if (nrComponents == 1)
        {
            internalFormat = dataFormat = GL_RED;
        }
        else if (nrComponents == 3)
        {
            internalFormat = gammaCorrection ? GL_SRGB : GL_RGB;
            dataFormat = GL_RGB;
        }
        else if (nrComponents == 4)
        {
            internalFormat = gammaCorrection ? GL_SRGB_ALPHA : GL_RGBA;
            dataFormat = GL_RGBA;
        }

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
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

// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
                // positions        // texture Coords
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
                1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

// renderCube() renders a 1x1 3D cube in NDC.
// -------------------------------------------------
unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube()
{
    // initialize (if necessary)
    if (cubeVAO == 0)
    {
        float vertices[] = {
                // back face
                -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
                1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
                1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right
                1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
                -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
                -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
                // front face
                -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
                1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
                1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
                1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
                -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
                -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
                // left face
                -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
                -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
                -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
                -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
                -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
                -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
                // right face
                1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
                1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
                1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right
                1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
                1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
                1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left
                // bottom face
                -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
                1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
                1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
                1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
                -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
                -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
                // top face
                -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
                1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
                1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right
                1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
                -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
                -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}