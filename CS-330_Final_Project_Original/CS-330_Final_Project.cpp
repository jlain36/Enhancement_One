/* CS-330_Final_Project.cpp
   Author: John Lain
   Date: 4/16/2021
*/

// Pre-processor directives
#include <vector>
#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library
#include <math.h>
// Needed for the image library
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>      // Image loading Utility functions

// GLM Math Header inclusions
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Header file modified to incorporate UP/DOWN enums
#include <learnOpengl/camera.h> // Camera class

// This is a Microsoft implement
using namespace std;

// Shader program Macro
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
    const char* const WINDOW_TITLE = "CS-330 Final Project"; // Macro for window title

    // Variables for window width and height
    const int WINDOW_WIDTH = 1600;
    const int WINDOW_HEIGHT = 1000;

    // Stores the GL data relative to a given mesh
    struct GLMesh
    {
        GLuint vao;         // Handle for the vertex array object
        GLuint vbo;         // VBO 1 for the Desktop / chair / plane / books objects
        GLuint vbo2;        // VBO 2 for the globe object
        GLuint vbo3;        // VBO 3 for the globe base object
        GLuint nIndices;    // Number of indices of the meshes
    };

    // Initialize variables
    //-----------------------
    // Main GLFW window
    GLFWwindow* gWindow = nullptr;
    // Triangle mesh data
    GLMesh gMesh;
    // Triangle mesh data
    GLMesh gGlobeMesh;
    // Triangle mesh data
    GLMesh gBaseMesh;
    // Shader program
    GLuint gProgramId;
    // Lamp Shader program
    GLuint gLampProgramId;
    // Texture id
    GLuint gTextureIdDeskChair;
    // Texture id
    GLuint gTextureIdBook1;
    // Texture id
    GLuint gTextureIdBook2;
    // Texture id
    GLuint gTextureIdPlane;
    // Texture id
    GLuint gTextureIdGlobe;
    // Texture id
    GLuint gTextureIdBase;
    // Texture id
    GLuint gTextureIdLegs;

    // Set texture coords. scale
    glm::vec2 gUVScale(0.2f, 0.2f);

    // Camera
    Camera gCamera(glm::vec3(0.0f, 0.5f, 6.5f));
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;

    // Timing
    float gDeltaTime = 0.0f; // time between current frame and last frame
    float gLastFrame = 0.0f;

    // Light color / position / scale
    glm::vec3 gObjectColor(1.f, 0.2f, 0.0f);
    glm::vec3 gLightColor(0.6f, 0.6f, 0.6f);
    glm::vec3 gLightPosition(-10.0f, 10.0f, 3.0f);
    glm::vec3 gLightScale(0.3f);

    // Light 2 color / position / scale
    glm::vec3 gLightColor2(0.0f, 0.45f, 0.45f);
    glm::vec3 gLightPosition2(10.0f, 1.5f, 3.0f);
    glm::vec3 gLightScale2(0.3f);
}

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */

 // Function prototypes
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UCreateMesh(GLMesh& desktopmesh);
void UDestroyMesh(GLMesh& desktopmesh);
void UCreateGlobeMesh(GLMesh& globemesh);
void UDestroyGlobeMesh(GLMesh& globemesh);
void UCreateBaseMesh(GLMesh& basemesh);
void UDestroyBaseMesh(GLMesh& basemesh);
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);

// Primitive function prototypes
void Desktop();
void Book_1();
void Book_2();
void Plane();
void Desk_Leg_1();
void Desk_Leg_2();
void Desk_Leg_3();
void Desk_Leg_4();
void Desk_Leg_Support_1();
void Desk_Leg_Support_2();
void Chair_Back();
void Chair_Seat();
void Chair_Back_Support_1();
void Chair_Back_Support_2();
void Chair_Seat_Support_1();
void Chair_Seat_Support_2();
void Chair_Leg_1();
void Chair_Leg_2();
void Chair_Leg_Support_1();
void Chair_Leg_Support_2();
void Globe();
void Base();

// Change the scene view
bool changePerspective = false;

// Vertex Shader Source Code
const GLchar* vertexShaderSource = GLSL(440,
    layout(location = 0) in vec3 position;
    layout(location = 1) in vec3 normal; // VAP position 1 for normals
    layout(location = 2) in vec2 textureCoordinate;

    out vec3 vertexNormal; // For outgoing normals to fragment shader
    out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
    out vec2 vertexTextureCoordinate;

    //Global variables for the transform matrices
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main()
    {
        gl_Position = projection * view * model * vec4(position, 1.0f); // transforms vertices to clip coordinates
        vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

        vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
        vertexTextureCoordinate = textureCoordinate;
    }
);

// Fragment Shader Source Code
const GLchar* fragmentShaderSource = GLSL(440,
    in vec3 vertexNormal; // For incoming normals
    in vec3 vertexFragmentPos; // For incoming fragment position
    in vec2 vertexTextureCoordinate;

    out vec4 fragmentColor;

    // Uniform variables for lighting
    // Light # 1
    uniform vec3 objectColor;
    uniform vec3 lightColor;
    uniform vec3 lightPos;
    uniform vec3 viewPosition;
    uniform vec2 uvScale;
   
    // Light # 2
    uniform vec3 lightColor2;
    uniform vec3 lightPos2;
    uniform vec3 viewPosition2;

    // Uniform variables for the textures
    uniform sampler2D uDeskChair;
    uniform sampler2D uBook1;
    uniform sampler2D uPlane;
    uniform sampler2D uBook2;
    uniform sampler2D uGlobe;
    uniform sampler2D uBase;
    uniform sampler2D uLegs;

    void main()
    {
        // Phong lighting model calculations to generate ambient, diffuse, and specular components

        //Calculate Ambient lighting for light # 1
        float ambientStrength = 0.6f; // Set ambient or global lighting strength
        vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

        //Calculate Diffuse lighting for light # 1
        vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
        vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
        float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
        vec3 diffuse = impact * lightColor; // Generate diffuse light color

        //Calculate Specular lighting for light # 1
        float specularIntensity = 0.7f; // Set specular light strength
        float highlightSize = 10.0f; // Set specular highlight size
        vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
        vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
        //Calculate specular component
        float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
        vec3 specular = specularIntensity * specularComponent * lightColor;

        //Calculate Ambient lighting for light # 2
        float ambientStrength2 = 0.6f; // Set ambient or global lighting strength
        vec3 ambient2 = ambientStrength2 * lightColor2; // Generate ambient light color

        //Calculate Diffuse lighting for light # 2
        vec3 lightDirection2 = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
        float impact2 = max(dot(norm, lightDirection2), 0.0);// Calculate diffuse impact by generating dot product of normal and light
        vec3 diffuse2 = impact2 * lightColor2; // Generate diffuse light color

        //Calculate Specular lighting for light # 2
        float specularIntensity2 = 0.1f; // Set specular light strength
        float highlightSize2 = 10.0f; // Set specular highlight size
      //  vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
        vec3 reflectDir2 = reflect(-lightDirection2, norm);// Calculate reflection vector
        //Calculate specular component
        float specularComponent2 = pow(max(dot(viewDir, reflectDir2), 0.0), highlightSize2);
        vec3 specular2 = specularIntensity2 * specularComponent2 * lightColor2;

        // Calculate phong result
        vec3 objectColor = texture(uDeskChair, vertexTextureCoordinate).xyz;
        vec3 Result = (ambient + diffuse + specular);
        vec3 Result2 = (ambient2 + diffuse2 + specular2);
        vec3 lightingResult = Result + Result2;
        vec3 phong = (lightingResult) * objectColor;
        fragmentColor = vec4(phong, 1.0); // Send lighting results to GPU
    }
);

// Lamp Shader Source Code
const GLchar* lampVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data

    //Uniform / Global variables for the  transform matrices
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main()
    {
        gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
    }
);


// Fragment Shader Source Code
const GLchar* lampFragmentShaderSource = GLSL(440,

    out vec4 fragmentColor; // For outgoing lamp color (smaller cube) to the GPU

    void main()
    {
        fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
    }
);

// Flips the image 180 degrees
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
    for (int j = 0; j < height / 2; ++j)
    {
        int index1 = j * width * channels;
        int index2 = (height - 1 - j) * width * channels;

        for (int i = width * channels; i > 0; --i)
        {
            unsigned char tmp = image[index1];
            image[index1] = image[index2];
            image[index2] = tmp;
            ++index1;
            ++index2;
        }
    }
}

// Main function 
int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
        return EXIT_FAILURE;

    // Calls the functions to create the Vertex Buffer Objects
    // Create the desktop mesh
    UCreateMesh(gMesh);
    // Create the globe mesh
    UCreateGlobeMesh(gGlobeMesh);
    // Create the globe base mesh
    UCreateBaseMesh(gBaseMesh);

    // Create the shader program
    if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, gProgramId))
        return EXIT_FAILURE;

    // Create the shader program
    if (!UCreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource, gLampProgramId))
        return EXIT_FAILURE;

    // Load textures
    const char* texFilename = "Images/desk_chair.png";
    if (!UCreateTexture(texFilename, gTextureIdDeskChair))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }

    texFilename = "Images/book1.png";
    if (!UCreateTexture(texFilename, gTextureIdBook1))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }

    texFilename = "Images/plane.png";
    if (!UCreateTexture(texFilename, gTextureIdPlane))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }

    texFilename = "Images/book2.png";
    if (!UCreateTexture(texFilename, gTextureIdBook2))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }

    texFilename = "Images/globe.png";
    if (!UCreateTexture(texFilename, gTextureIdGlobe))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }

    texFilename = "Images/base.png";
    if (!UCreateTexture(texFilename, gTextureIdBase))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }

    texFilename = "Images/leg_color.png";
    if (!UCreateTexture(texFilename, gTextureIdLegs))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }

    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gProgramId);

    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gProgramId, "uDeskChair"), 0);

    // We set the texture as texture unit 1
    glUniform1i(glGetUniformLocation(gProgramId, "uBook1"), 1);

    // We set the texture as texture unit 2
    glUniform1i(glGetUniformLocation(gProgramId, "uPlane"), 2);

    // We set the texture as texture unit 3
    glUniform1i(glGetUniformLocation(gProgramId, "uBook2"), 3);

    // We set the texture as texture unit 4
    glUniform1i(glGetUniformLocation(gProgramId, "uGlobe"), 4);

    // We set the texture as texture unit 5
    glUniform1i(glGetUniformLocation(gProgramId, "uBase"), 5);

    // Sets the background color of the window to black (it will be implicitely used by glClear)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // render loop
    // -----------
    // Render until the window is closed
    while (!glfwWindowShouldClose(gWindow))
    {
        // per-frame timing
        // --------------------
        float currentFrame = glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;

        // input
        // -----
        UProcessInput(gWindow);

        // Enable z-depth
        glEnable(GL_DEPTH_TEST);

        // Clear the frame and z buffers
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Renders the scene objects
        Desktop();
        Book_1();
        Book_2();
        Plane();
        Desk_Leg_1();
        Desk_Leg_2();
        Desk_Leg_3();
        Desk_Leg_4();
        Desk_Leg_Support_1();
        Desk_Leg_Support_2();
        Chair_Back();
        Chair_Seat();
        Chair_Back_Support_1();
        Chair_Back_Support_2();
        Chair_Seat_Support_1();
        Chair_Seat_Support_2();
        Chair_Leg_1();
        Chair_Leg_2();
        Chair_Leg_Support_1();
        Chair_Leg_Support_2();
        Globe();
        Base();
        

        /* Flips the the back buffer with the front buffer every frame.
           Only needs to be called once per loop iteration */
        glfwSwapBuffers(gWindow);

        // Check status
        glfwPollEvents();
    }

    // Release mesh data
    UDestroyMesh(gMesh);
    UDestroyGlobeMesh(gGlobeMesh);
    UDestroyBaseMesh(gBaseMesh);

    // Release texture data
    UDestroyTexture(gTextureIdDeskChair);
    UDestroyTexture(gTextureIdBook1);
    UDestroyTexture(gTextureIdBook2);
    UDestroyTexture(gTextureIdPlane);
    UDestroyTexture(gTextureIdGlobe);
    UDestroyTexture(gTextureIdBase);

    // Release shader programs
    UDestroyShaderProgram(gProgramId);
    UDestroyShaderProgram(gLampProgramId);

    // Terminates the program successfully
    exit(EXIT_SUCCESS);
}

// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    // GLFW: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Use Multi-sample anti-aliasing with 5 samples
    glfwWindowHint(GLFW_SAMPLES, 5);

    // Enable MSAA
    glEnable(GL_MULTISAMPLE);

    // Sets forward compatibility
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // GLFW: window creation
    // ---------------------
    * window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);
    glfwSetCursorPosCallback(*window, UMousePositionCallback);
    glfwSetScrollCallback(*window, UMouseScrollCallback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLEW: initialize
    // ----------------
    // Note: if using GLEW version 1.13 or earlier
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult)
    {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return false;
    }

    // Displays GPU OpenGL version
    cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

    return true;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
    // Define and initialize the camera speed
    static const float cameraSpeed = 2.5f;

    changePerspective = false;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Keyboard input
    //--------------------
    // Moves objects closer to viewer
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        gCamera.ProcessKeyboard(FORWARD, gDeltaTime);

    // Moves objects further from viewer
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);

    // Moves objects to the right
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        gCamera.ProcessKeyboard(LEFT, gDeltaTime);

    // Moves objects to the left
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        gCamera.ProcessKeyboard(RIGHT, gDeltaTime);

    // Moves objects up
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        gCamera.ProcessKeyboard(UP, gDeltaTime);

    // Moves objects down
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        gCamera.ProcessKeyboard(DOWN, gDeltaTime);

    // Changes the viewport perspective 
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
        changePerspective = true;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (gFirstMouse)
    {
        gLastX = xpos;
        gLastY = ypos;
        gFirstMouse = false;
    }

    float xoffset = xpos - gLastX;
    float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

    xoffset += gCamera.MouseSensitivity;
    yoffset += gCamera.MouseSensitivity;

  //  gCamera.Yaw += xoffset;
  //  gCamera.Pitch += yoffset;

    gLastX = xpos;
    gLastY = ypos;

    gCamera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    gCamera.ProcessMouseScroll(yoffset);
}

// Desktop function
void Desktop()
{
    // Position variables
    const float xsize = 0.0f;
    const float ysize = 0.0f;
    const float zsize = 0.0f;

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    // Perspective projection
    if (changePerspective != true)
    {
        glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Orthographic projection
    else if (changePerspective == true)
    {
        glm::mat4 projection = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao);

    // Bind texture unit 0
    glBindTexture(GL_TEXTURE_2D, gTextureIdDeskChair);

    // 1. Scales the object
    glm::mat4 scale = glm::scale(glm::vec3(1.5f, 0.1f, 2.0f));

    // 2. Rotates shape by 15 degrees in the x axis
    glm::mat4 rotation = glm::rotate(45.0f, glm::vec3(0.0, 0.1f, 0.0f));

    // Location currently se at the origin
    glm::vec3 location = glm::vec3(xsize, ysize, zsize);
    // 3. Place object at the origin
    glm::mat4 translation = glm::translate(location);
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    GLint objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    GLint lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    GLint lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    GLint viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
    const glm::vec3 cameraPosition = gCamera.Position;
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    GLint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nIndices);

    // LAMP: draw lamp
    //----------------
    glUseProgram(gLampProgramId);

    //Transform the smaller cube used as a visual que for the light source
    model = glm::translate(gLightPosition) * glm::scale(gLightScale);

    // Reference matrix uniforms from the Lamp Shader program
    modelLoc = glGetUniformLocation(gLampProgramId, "model");
    viewLoc = glGetUniformLocation(gLampProgramId, "view");
    projLoc = glGetUniformLocation(gLampProgramId, "projection");

    // Pass matrix data to the Lamp Shader program's matrix uniforms
    glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glDrawArrays(GL_TRIANGLES, 0, gMesh.nIndices);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);
    glUseProgram(0);
}

// Book 1 function
void Book_1()
{
    // Position variables
    const float xsize = 0.5f;
    const float ysize = 0.1f;
    const float zsize = 0.1f;

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    // Perspective projection
    if (changePerspective != true)
    {
        glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Orthographic projection
    else if (changePerspective == true)
    {
        glm::mat4 projection = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao);

    // Bind texture unit 1
    glBindTexture(GL_TEXTURE_2D, gTextureIdBook1);

    // 1. Scales the object
    glm::mat4 scale = glm::scale(glm::vec3(0.7f, 0.1f, 0.45f));

    // 2. Rotates shape by 149 degrees in the x axis
    glm::mat4 rotation = glm::rotate(149.0f, glm::vec3(0.0, 0.1f, 0.0f));

    // Location currently set at the origin
    glm::vec3 location = glm::vec3(xsize, ysize, zsize);
    // 3. Place object at the origin
    glm::mat4 translation = glm::translate(location);
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    GLint objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    GLint lightColorLoc = glGetUniformLocation(gProgramId, "lightColor2");
    GLint lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos2");
    GLint viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition2");

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor2.r, gLightColor2.g, gLightColor2.b);
    glUniform3f(lightPositionLoc, gLightPosition2.x, gLightPosition2.y, gLightPosition2.z);
    const glm::vec3 cameraPosition = gCamera.Position;
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    GLint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nIndices);

    // LAMP: draw lamp
    //----------------
    glUseProgram(gLampProgramId);

    //Transform the smaller cube used as a visual que for the light source
    model = glm::translate(gLightPosition2) * glm::scale(gLightScale2);

    // Reference matrix uniforms from the Lamp Shader program
    modelLoc = glGetUniformLocation(gLampProgramId, "model");
    viewLoc = glGetUniformLocation(gLampProgramId, "view");
    projLoc = glGetUniformLocation(gLampProgramId, "projection");

    // Pass matrix data to the Lamp Shader program's matrix uniforms
    glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glDrawArrays(GL_TRIANGLES, 0, gMesh.nIndices);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);
    glUseProgram(0);
}

// Book 2 function
void Book_2()
{
    // Position variables
    const float xsize = 0.5f;
    const float ysize = 0.2f;
    const float zsize = 0.1f;

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    // Perspective projection
    if (changePerspective != true)
    {
        glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Orthographic projection
    else if (changePerspective == true)
    {
        glm::mat4 projection = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao);

    // Bind texture unit 3
    glBindTexture(GL_TEXTURE_2D, gTextureIdBook2);

    // 1. Scales the object 
    glm::mat4 scale = glm::scale(glm::vec3(0.7f, 0.1f, 0.45f));

    // 2. Rotates shape by 148.6 degrees in the x axis
    glm::mat4 rotation = glm::rotate(148.6f, glm::vec3(0.0, 0.1f, 0.0f));

    // Location currently set at the origin
    glm::vec3 location = glm::vec3(xsize, ysize, zsize);
    // 3. Place object at the origin
    glm::mat4 translation = glm::translate(location);
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    GLint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nIndices);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);
}

// Plane function
void Plane()
{
    // Position variables
    const float xsize = 0.0f;
    const float ysize = -1.157f;
    const float zsize = -1.5f;

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    // Perspective projection
    if (changePerspective != true)
    {
        glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Orthographic projection
    else if (changePerspective == true)
    {
        glm::mat4 projection = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao);

    // Bind texture unit 2
    glBindTexture(GL_TEXTURE_2D, gTextureIdPlane);

    // 1. Scales the object 
    glm::mat4 scale = glm::scale(glm::vec3(10.0f, 0.01f, 10.0f));

    // 2. Rotates shape by 45 degrees in the x axis
    glm::mat4 rotation = glm::rotate(45.0f, glm::vec3(0.0, 0.1f, 0.0f));

    // Location currently set at the origin
    glm::vec3 location = glm::vec3(xsize, ysize, zsize);
    // 3. Place object at the origin
    glm::mat4 translation = glm::translate(location);
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    GLint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nIndices);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);
}

// Desk leg 1 function
void Desk_Leg_1()
{
    // Position variables
    const float xsize = 1.07f;
    const float ysize = -0.56f;
    const float zsize = 0.0f;

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    // Perspective projection
    if (changePerspective != true)
    {
        glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Orthographic projection
    else if (changePerspective == true)
    {
        glm::mat4 projection = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao);

    // Bind texture unit 1
    glBindTexture(GL_TEXTURE_2D, gTextureIdLegs);

    // 1. Scales the object 
    glm::mat4 scale = glm::scale(glm::vec3(0.06f, 1.2f, 0.06f));

    // Rotate shape 45 degrees
    glm::mat4 rotation = glm::rotate(45.0f, glm::vec3(0.0, 0.1f, 0.0f));

    // Location currently set at the origin
    glm::vec3 location = glm::vec3(xsize, ysize, zsize);
    // 3. Place object at the origin
    glm::mat4 translation = glm::translate(location);
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nIndices);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);
}

// Desk leg 2 function
void Desk_Leg_2()
{
    // Position variables
    const float xsize = 0.454f;
    const float ysize = -0.56f;
    const float zsize = 1.0f;

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    // Perspective projection
    if (changePerspective != true)
    {
        glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Orthographic projection
    else if (changePerspective == true)
    {
        glm::mat4 projection = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao);

    // Bind texture unit 1
    glBindTexture(GL_TEXTURE_2D, gTextureIdLegs);

    // 1. Scales the object
    glm::mat4 scale = glm::scale(glm::vec3(0.06f, 1.2f, 0.06f));

    // Rotate shape 45 degrees
    glm::mat4 rotation = glm::rotate(45.0f, glm::vec3(0.0, 0.1f, 0.0f));

    // Location currently set at the origin
    glm::vec3 location = glm::vec3(xsize, ysize, zsize);
    // 3. Place object at the origin
    glm::mat4 translation = glm::translate(location);
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nIndices);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

}

// Desk leg 3 function
void Desk_Leg_3()
{
    // Position variables
    const float xsize = -0.355f;
    const float ysize = -0.56f;
    const float zsize = -1.0f;

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    // Perspective projection
    if (changePerspective != true)
    {
        glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Orthographic projection
    else if (changePerspective == true)
    {
        glm::mat4 projection = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao);

    // Bind texture unit 1
    glBindTexture(GL_TEXTURE_2D, gTextureIdLegs);

    // 1. Scales the object
    glm::mat4 scale = glm::scale(glm::vec3(0.06f, 1.2f, 0.06f));

    // Rotate shape 45 degrees
    glm::mat4 rotation = glm::rotate(45.0f, glm::vec3(0.0, 0.1f, 0.0f));

    // Location currently set at the origin
    glm::vec3 location = glm::vec3(xsize, ysize, zsize);
    // 3. Place object at the origin
    glm::mat4 translation = glm::translate(location);
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nIndices);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);
}

// Desk leg 4 function
void Desk_Leg_4()
{
    // Position variables
    const float xsize = -1.052f;
    const float ysize = -0.56f;
    const float zsize = 0.13f;

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    // Perspective projection
    if (changePerspective != true)
    {
        glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Orthographic projection
    else if (changePerspective == true)
    {
        glm::mat4 projection = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao);

    // Bind texture unit 1
    glBindTexture(GL_TEXTURE_2D, gTextureIdLegs);

    // 1. Scales the object
    glm::mat4 scale = glm::scale(glm::vec3(0.06f, 1.2f, 0.06f));

    // 2. Rotates shape by 45 degrees 
    glm::mat4 rotation = glm::rotate(45.0f, glm::vec3(0.0, 0.1f, 0.0f));

    // Location currently set at the origin
    glm::vec3 location = glm::vec3(xsize, ysize, zsize);
    // 3. Place object at the origin
    glm::mat4 translation = glm::translate(location);
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nIndices);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);
}

// Desk support leg 1 function
void Desk_Leg_Support_1()
{
    // Position variables
    const float xsize = 0.79f;
    const float ysize = -1.1f;
    const float zsize = 0.465f;

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    // Perspective projection
    if (changePerspective != true)
    {
        glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Orthographic projection
    else if (changePerspective == true)
    {
        glm::mat4 projection = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao);

    // Bind texture unit 1
    glBindTexture(GL_TEXTURE_2D, gTextureIdLegs);

    // 1. Scales the object 
    glm::mat4 scale = glm::scale(glm::vec3(1.55f, 0.1f, 0.05f));

    // 2. Rotates shape by 45 degrees
    glm::mat4 rotation = glm::rotate(45.0f, glm::vec3(0.0, 0.1f, 0.0f));

    // Location currently set at the origin
    glm::vec3 location = glm::vec3(xsize, ysize, zsize);
    // 3. Place object at the origin
    glm::mat4 translation = glm::translate(location);
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nIndices);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);
}

// Desk support leg 2 function
void Desk_Leg_Support_2()
{
    // Position variables
    const float xsize = -0.7f;
    const float ysize = -1.1f;
    const float zsize = -0.43f;

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    // Perspective projection
    if (changePerspective != true)
    {
        glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Orthographic projection
    else if (changePerspective == true)
    {
        glm::mat4 projection = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao);

    // Bind texture unit 1
    glBindTexture(GL_TEXTURE_2D, gTextureIdLegs);

    // 1. Scales the object
    glm::mat4 scale = glm::scale(glm::vec3(1.55f, 0.1f, 0.05f));

    // 2. Rotates shape by 45 degrees 
    glm::mat4 rotation = glm::rotate(45.0f, glm::vec3(0.0, 0.1f, 0.0f));

    // Location currently set at the origin
    glm::vec3 location = glm::vec3(xsize, ysize, zsize);
    // 3. Place object at the origin
    glm::mat4 translation = glm::translate(location);
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nIndices);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);
}

// Chair seat function
void Chair_Back()
{
    // Position variables
    const float xsize = -0.51f;
    const float ysize = 0.0f;
    const float zsize = 1.05f;

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    // Perspective projection
    if (changePerspective != true)
    {
        glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Orthographic projection
    else if (changePerspective == true)
    {
        glm::mat4 projection = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao);

    // Bind texture unit 1
    glBindTexture(GL_TEXTURE_2D, gTextureIdDeskChair);

    // 1. Scales the object
    glm::mat4 scale = glm::scale(glm::vec3(0.06f, 0.45f, 0.65f));

    // 2. Rotates shape by 45 degrees 
    glm::mat4 rotation = glm::rotate(45.0f, glm::vec3(0.0, 0.1f, 0.0f));

    // Location currently set at the origin
    glm::vec3 location = glm::vec3(xsize, ysize, zsize);
    // 3. Place object at the origin
    glm::mat4 translation = glm::translate(location);
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nIndices);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);
}

// Chair seat function
void Chair_Seat()
{
    // Position variables
    const float xsize = -0.35f;
    const float ysize = -0.41f;
    const float zsize = 0.8f;

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    // Perspective projection
    if (changePerspective != true)
    {
        glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Orthographic projection
    else if (changePerspective == true)
    {
        glm::mat4 projection = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao);

    // Bind texture unit 1
    glBindTexture(GL_TEXTURE_2D, gTextureIdDeskChair);

    // 1. Scales the object
    glm::mat4 scale = glm::scale(glm::vec3(0.65f, 0.06f, 0.65f));

    // 2. Rotates shape by 45 degrees 
    glm::mat4 rotation = glm::rotate(45.0f, glm::vec3(0.0, 0.1f, 0.0f));

    // Location currently set at the origin
    glm::vec3 location = glm::vec3(xsize, ysize, zsize);
    // 3. Place object at the origin
    glm::mat4 translation = glm::translate(location);
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nIndices);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);
}

// Chair back Support 1 function
void Chair_Back_Support_1()
{
    // Position variables
    const float xsize = -0.77f;
    const float ysize = -0.12f;
    const float zsize = 0.95f;

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    // Perspective projection
    if (changePerspective != true)
    {
        glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Orthographic projection
    else if (changePerspective == true)
    {
        glm::mat4 projection = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao);

    // Bind texture unit 1
    glBindTexture(GL_TEXTURE_2D, gTextureIdLegs);

    // 1. Scales the object
    glm::mat4 scale = glm::scale(glm::vec3(0.05f, 0.65f, 0.05f));

    // 2. Rotates shape by 45 degrees 
    glm::mat4 rotation = glm::rotate(45.0f, glm::vec3(0.0, 0.1f, 0.0f));

    // Location currently set at the origin
    glm::vec3 location = glm::vec3(xsize, ysize, zsize);
    // 3. Place object at the origin
    glm::mat4 translation = glm::translate(location);
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nIndices);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);
}

// Chair back Support 2 function
void Chair_Back_Support_2()
{
    // Position variables
    const float xsize = -0.32f;
    const float ysize = -0.12f;
    const float zsize = 1.25f;

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    // Perspective projection
    if (changePerspective != true)
    {
        glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Orthographic projection
    else if (changePerspective == true)
    {
        glm::mat4 projection = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao);

    // Bind texture unit 1
    glBindTexture(GL_TEXTURE_2D, gTextureIdLegs);

    // 1. Scales the object
    glm::mat4 scale = glm::scale(glm::vec3(0.05f, 0.65f, 0.05f));

    // 2. Rotates shape by 45 degrees
    glm::mat4 rotation = glm::rotate(45.0f, glm::vec3(0.0, 0.1f, 0.0f));

    // Location currently set at the origin
    glm::vec3 location = glm::vec3(xsize, ysize, zsize);
    // 3. Place object at the origin
    glm::mat4 translation = glm::translate(location);
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nIndices);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);
}

// Chair seat support 1 function
void Chair_Seat_Support_1()
{
    // Position variables
    const float xsize = -0.160f;
    const float ysize = -0.46f;
    const float zsize = 0.992f;

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    // Perspective projection
    if (changePerspective != true)
    {
        glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Orthographic projection
    else if (changePerspective == true)
    {
        glm::mat4 projection = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao);

    // Bind texture unit 1
    glBindTexture(GL_TEXTURE_2D, gTextureIdLegs);

    // 1. Scales the object
    glm::mat4 scale = glm::scale(glm::vec3(0.65f, 0.05f, 0.05f));

    // 2. Rotates shape by 45 degrees 
    glm::mat4 rotation = glm::rotate(45.0f, glm::vec3(0.0, 0.1f, 0.0f));

    // Location currently set at the origin
    glm::vec3 location = glm::vec3(xsize, ysize, zsize);
    // 3. Place object at the origin
    glm::mat4 translation = glm::translate(location);
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nIndices);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);
}

// Chair seat support 1 function
void Chair_Seat_Support_2()
{
    // Position variables
    const float xsize = -0.614f;
    const float ysize = -0.46f;
    const float zsize = 0.691f;

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    // Perspective projection
    if (changePerspective != true)
    {
        glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Orthographic projection
    else if (changePerspective == true)
    {
        glm::mat4 projection = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao);

    // Bind texture unit 1
    glBindTexture(GL_TEXTURE_2D, gTextureIdLegs);

    // 1. Scales the object
    glm::mat4 scale = glm::scale(glm::vec3(0.65f, 0.05f, 0.05f));

    // 2. Rotates shape by 45 degrees 
    glm::mat4 rotation = glm::rotate(45.0f, glm::vec3(0.0, 0.1f, 0.0f));

    // Location currently set at the origin
    glm::vec3 location = glm::vec3(xsize, ysize, zsize);
    // 3. Place object at the origin
    glm::mat4 translation = glm::translate(location);
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nIndices);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);
}

// Desk leg 1 function
void Chair_Leg_1()
{
    // Position variables
    const float xsize = -0.155f;
    const float ysize = -0.79f;
    const float zsize = 0.99f;

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    // Perspective projection
    if (changePerspective != true)
    {
        glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Orthographic projection
    else if (changePerspective == true)
    {
        glm::mat4 projection = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao);

    // Bind texture unit 1
    glBindTexture(GL_TEXTURE_2D, gTextureIdLegs);

    // 1. Scales the object 
    glm::mat4 scale = glm::scale(glm::vec3(0.05f, 0.68f, 0.04f));

    // 2. Rotates shape by 45 degrees 
    glm::mat4 rotation = glm::rotate(45.0f, glm::vec3(0.0, 0.1f, 0.0f));

    // Location currently set at the origin
    glm::vec3 location = glm::vec3(xsize, ysize, zsize);
    // 3. Place object at the origin
    glm::mat4 translation = glm::translate(location);
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nIndices);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);
}

// Desk leg 1 function
void Chair_Leg_2()
{
    // Position variables
    const float xsize = -0.62f;
    const float ysize = -0.79f;
    const float zsize = 0.69f;

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    // Perspective projection
    if (changePerspective != true)
    {
        glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Orthographic projection
    else if (changePerspective == true)
    {
        glm::mat4 projection = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao);

    // Bind texture unit 1
    glBindTexture(GL_TEXTURE_2D, gTextureIdLegs);

    // 1. Scales the object 
    glm::mat4 scale = glm::scale(glm::vec3(0.05f, 0.68f, 0.04f));

    // 2. Rotates shape by 45 degrees 
    glm::mat4 rotation = glm::rotate(45.0f, glm::vec3(0.0, 0.1f, 0.0f));

    // Location currently set at the origin
    glm::vec3 location = glm::vec3(xsize, ysize, zsize);
    // 3. Place object at the origin
    glm::mat4 translation = glm::translate(location);
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nIndices);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);
}

// Desk support leg 1 function
void Chair_Leg_Support_1()
{
    // Position variables
    const float xsize = -0.160f;
    const float ysize = -1.1f;
    const float zsize = 0.99f;

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    // Perspective projection
    if (changePerspective != true)
    {
        glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Orthographic projection
    else if (changePerspective == true)
    {
        glm::mat4 projection = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao);

    // Bind texture unit 1
    glBindTexture(GL_TEXTURE_2D, gTextureIdLegs);

    // 1. Scales the object by 2
    glm::mat4 scale = glm::scale(glm::vec3(0.55f, 0.1f, 0.05f));

    // 2. Rotates shape by 45 degrees 
    glm::mat4 rotation = glm::rotate(45.0f, glm::vec3(0.0, 0.1f, 0.0f));

    // Location currently set at the origin
    glm::vec3 location = glm::vec3(xsize, ysize, zsize);
    // 3. Place object at the origin
    glm::mat4 translation = glm::translate(location);
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nIndices);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);
}

// Desk support leg 1 function
void Chair_Leg_Support_2()
{
    // Position variables
    const float xsize = -0.62f;
    const float ysize = -1.1f;
    const float zsize = 0.69f;

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    // Perspective projection
    if (changePerspective != true)
    {
        glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Orthographic projection
    else if (changePerspective == true)
    {
        glm::mat4 projection = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao);

    // Bind texture unit 1
    glBindTexture(GL_TEXTURE_2D, gTextureIdLegs);

    // 1. Scales the object by 2
    glm::mat4 scale = glm::scale(glm::vec3(0.55f, 0.1f, 0.05f));

    // 2. Rotates shape by 45 degrees 
    glm::mat4 rotation = glm::rotate(45.0f, glm::vec3(0.0, 0.1f, 0.0f));

    // Location currently set at the origin
    glm::vec3 location = glm::vec3(xsize, ysize, zsize);
    // 3. Place object at the origin
    glm::mat4 translation = glm::translate(location);
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nIndices);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);
}

// Globe function
void Globe()
{
    // Position variables
    const float xsize = -0.485f;
    const float ysize = 0.60f;
    const float zsize = -0.45f;

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    // Perspective projection
    if (changePerspective != true)
    {
        glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Orthographic projection
    else if (changePerspective == true)
    {
        glm::mat4 projection = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gGlobeMesh.vao);

    // Bind texture unit 4
    glBindTexture(GL_TEXTURE_2D, gTextureIdGlobe);

    // 1. Scales the object
    glm::mat4 scale = glm::scale(glm::vec3(0.4f, 0.4f, 0.4f));

    // 2. Rotates shape by 0 degrees in the x axis
    glm::mat4 rotation = glm::rotate(0.0f, glm::vec3(0.0, 0.1f, 0.0f));

    // Location currently set at the origin
    glm::vec3 location = glm::vec3(xsize, ysize, zsize);
    // 3. Place object at the origin
    glm::mat4 translation = glm::translate(location);
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    GLint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gGlobeMesh.nIndices);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);
}

// Globe base function
void Base()
{
    // Position variables
    const float xsize = -0.47f;
    const float ysize = 0.128f;
    //const float ysize = 0.12f;
    const float zsize = -0.4f;

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    // Perspective projection
    if (changePerspective != true)
    {
        glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Orthographic projection
    else if (changePerspective == true)
    {
        glm::mat4 projection = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gBaseMesh.vao);

    // Bind texture unit 5
    glBindTexture(GL_TEXTURE_2D, gTextureIdBase);

    // 1. Scales the object 
    glm::mat4 scale = glm::scale(glm::vec3(0.04f, 0.04f, 0.04f));

    // 2. Rotates shape 
    glm::mat4 rotation = glm::rotate(0.0f, glm::vec3(0.0, 0.0f, 0.1f));
   
    // Location currently set at the origin
    glm::vec3 location = glm::vec3(xsize, ysize, zsize);
    // 3. Place object at the origin
    glm::mat4 translation = glm::translate(location);
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gBaseMesh.nIndices);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);
}

// Function to create the desktop mesh
void UCreateMesh(GLMesh& mesh)
{
    // Position and Color data
    GLfloat verts[] = {
        //Positions          //Normals          //Textures
        // ------------------------------------------------------
        //Back Face          //Negative Z Normal  Texture Coords.
       -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
        0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
        0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
       -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
       -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

        //Front Face         //Positive Z Normal
       -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
        0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
       -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
       -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

        //Left Face          //Negative X Normal
       -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
       -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
       -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
       -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
       -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
       -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

        //Right Face         //Positive X Normal
        0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

        //Bottom Face        //Negative Y Normal
       -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
        0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
       -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
       -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

        //Top Face         //Positive Y Normal
       -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
        0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
       -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
       -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
    };

    // Index data to share position data
    GLushort indices[] = {
        0, 1, 3,  // Triangle 1
        1, 2, 3,  // Triangle 2
        0, 1, 4,  // Triangle 3
        0, 4, 5,  // Triangle 4
        0, 5, 6,  // Triangle 5
        0, 3, 6,  // Triangle 6
        4, 5, 6,  // Triangle 7
        4, 6, 7,  // Triangle 8
        2, 3, 6,  // Triangle 9
        2, 6, 7,  // Triangle 10
        1, 4, 7,  // Triangle 11
        1, 2, 7   // Triangle 12
    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    mesh.nIndices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}

// Function to create the globe mesh
void UCreateGlobeMesh(GLMesh& globemesh)
{
    int i, j;
    std::vector<GLfloat> vertices;
    std::vector<GLuint> indices;
    std::vector<GLfloat> texCoords;
    int indicator = 0;
    int lats = 300;
    int longs = 300;
    float s = 0.0;
    float t = 0.0;

    for (i = 0; i <= lats; i++)
    {
        glColor4f(0.0, 1.0, 0.0, 0.0);
        float lat0 = glm::pi<float>() * (-0.5 + (float)(i - 1) / lats);
        float z0 = sin(lat0);
        float zr0 = cos(lat0);

        float lat1 = glm::pi<float>() * (-0.5 + (float)i / lats);
        float z1 = sin(lat1);
        float zr1 = cos(lat1);

        for (j = 0; j <= longs; j++)
        {
            float lng = 2 * glm::pi<float>() * (float)(j - 1) / longs;
            float x = cos(lng);
            float y = sin(lng);

            vertices.push_back(x * zr0);
            vertices.push_back(y * zr0);
            vertices.push_back(z0);
            indices.push_back(indicator);
            indicator++;

            vertices.push_back(x * zr1);
            vertices.push_back(y * zr1);
            vertices.push_back(z1);
            indices.push_back(indicator);
            indicator++;

            // vertex tex coord (s, t) range between [0, 1]
            s = (float)j / longs;
            t = (float)i / lats;
            texCoords.push_back(s);
            texCoords.push_back(t);
        }

        indices.push_back(GL_PRIMITIVE_RESTART_FIXED_INDEX);
    }

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    globemesh.nIndices = indices.size();

    glGenVertexArrays(1, &globemesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(globemesh.vao);

    // Create VBO 2
    glGenBuffers(1, &globemesh.vbo2);
    glBindBuffer(GL_ARRAY_BUFFER, globemesh.vbo2); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), &vertices[0], GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Strides between vertex coordinates
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}

// Function to create the globe base mesh
void UCreateBaseMesh(GLMesh& basemesh)
{
    const float PI = 3.1415926535897932384626433f;
    float sectorCount = 500;
    float sectorStep = 2 * PI / sectorCount;
    float sectorAngle;  // Radians
    float height = 4;
    float radius = 2;

    std::vector<GLfloat> unitCircleVertices;
    for (int i = 0; i <= sectorCount; ++i)
    {
        sectorAngle = i * sectorStep;
        unitCircleVertices.push_back(cos(sectorAngle)); // x
        unitCircleVertices.push_back(sin(sectorAngle)); // y
        unitCircleVertices.push_back(0);                // z
    }

    // Generate vertices for a cylinder
    std::vector<GLfloat> vertices;
    std::vector<GLfloat> normals;
    std::vector<GLfloat> texCoords;

    // Get unit circle vectors on XY-plane
    std::vector<GLfloat> unitVertices = unitCircleVertices;

    // Put side vertices to arrays
    for (int i = 0; i < 2; ++i)
    {
        float h = -height / 2.0f + i * height;           // z value; -h/2 to h/2
        float t = 1.0f - i;                              // Vertical tex coord; 1 to 0

        for (int j = 0, k = 0; j <= sectorCount; ++j, k += 3)
        {
            float ux = unitVertices[k];
            float uy = unitVertices[k + 1];
            float uz = unitVertices[k + 2];
            // Position vector
            vertices.push_back(ux * radius);             // vx
            vertices.push_back(uy * radius);             // vy
            vertices.push_back(h);                       // vz
            // Normal vector
            normals.push_back(ux);                       // nx
            normals.push_back(uy);                       // ny
            normals.push_back(uz);                       // nz
            // Texture coordinate
            texCoords.push_back((float)j / sectorCount); // s
            texCoords.push_back(t);                      // t
        }
    }

    // The starting index for the base/top surface
    // NOTE: it is used for generating indices later
    int baseCenterIndex = (int)vertices.size() / 3;
    int topCenterIndex = baseCenterIndex + sectorCount + 1; // Include center vertex

    // Put base and top vertices to arrays
    for (int i = 0; i < 2; ++i)
    {
        float h = -height / 2.0f + i * height;           // z value; -h/2 to h/2
        float nz = -1 + i * 2;                           // z value of normal; -1 to 1

        // Center point
        vertices.push_back(0);     vertices.push_back(0);     vertices.push_back(h);
        normals.push_back(0);      normals.push_back(0);      normals.push_back(nz);
        texCoords.push_back(0.5f); texCoords.push_back(0.5f);

        for (int j = 0, k = 0; j < sectorCount; ++j, k += 3)
        {
            float ux = unitVertices[k];
            float uy = unitVertices[k + 1];
            // Position vector
            vertices.push_back(ux * radius);             // vx
            vertices.push_back(uy * radius);             // vy
            vertices.push_back(h);                       // vz
            // Normal vector
            normals.push_back(0);                        // nx
            normals.push_back(0);                        // ny
            normals.push_back(nz);                       // nz
            // Texture coordinate
            texCoords.push_back(-ux * 0.5f + 0.5f);      // s
            texCoords.push_back(-uy * 0.5f + 0.5f);      // t
        }
    }

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    basemesh.nIndices = vertices.size();

    glGenVertexArrays(1, &basemesh.vao); // We can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(basemesh.vao);

    // Create VBO 3
    glGenBuffers(1, &basemesh.vbo3);
    glBindBuffer(GL_ARRAY_BUFFER, basemesh.vbo3); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), &vertices[0], GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Strides between vertex coordinates
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}

// Generate and load the texture
bool UCreateTexture(const char* filename, GLuint& textureId)
{
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (image)
    {
        flipImageVertically(image, width, height, channels);

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            cout << "Not implemented to handle image with " << channels << " channels" << endl;
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

        return true;
    }

    // Error loading the image
    return false;
}

// Destroy the texture on exit
void UDestroyTexture(GLuint textureId)
{
    glGenTextures(1, &textureId);
}

// Destroy the mesh on exit
void UDestroyMesh(GLMesh& mesh)
{
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(1, &mesh.vbo);
}

// Destroy the mesh on exit
void UDestroyGlobeMesh(GLMesh& globemesh)
{
    glDeleteVertexArrays(1, &globemesh.vao);
    glDeleteBuffers(1, &globemesh.vbo2);
}

// Destroy the mesh on exit
void UDestroyBaseMesh(GLMesh& basemesh)
{
    glDeleteVertexArrays(1, &basemesh.vao);
    glDeleteBuffers(1, &basemesh.vbo3);
}

// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // compile the vertex shader
    // check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // compile the fragment shader
    // check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);   // links the shader program
    // check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);    // Uses the shader program

    return true;
}

// Destroy the shader program on exit
void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}
