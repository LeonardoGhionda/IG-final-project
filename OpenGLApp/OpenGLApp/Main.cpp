#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "camera.h"
#include "model.h"
#include "ingredient.h"
#include "vecplus.h"
#include "screen.h"
#include "keys.h"

#include <iostream>
#include <random>
#include <deque>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// camera
//Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = screen.w / 2.0f;
float lastY = screen.h / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

glm::vec2 mousePos = glm::vec2(0.0f);

Keys keys;

std::random_device rd;
std::mt19937 gen(rd());

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
    GLFWwindow* window = glfwCreateWindow(screen.w, screen.h, "Model_Loader", NULL, NULL);
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

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    //stbi_set_flip_vertically_on_load(true);

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader ourShader("shader.vs", "shader.fs");

    // load models
    // -----------
    Model backgroundPlane("resources/background/background.obj");


    deque<Ingredient> ingredients;
    ingredients.push_back(Ingredient("resources/ball/ball.obj", Ingredient::RandomSpawnPoint()));
    ingredients.push_back(Ingredient("resources/ball/ball.obj", Ingredient::RandomSpawnPoint()));
    ingredients.push_back(Ingredient("resources/ball/ball.obj", Ingredient::RandomSpawnPoint()));
    ingredients.push_back(Ingredient("resources/ball/ball.obj", Ingredient::RandomSpawnPoint()));

    Ingredient* active = &ingredients[0]; //pointer to the current active ingredient
    active->AddVelocity(active->getDirectionToCenter() * 5.0f);
    

    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glDisable(GL_CULL_FACE);
    //glCullFace(GL_BACK); // Cull back faces
    //glFrontFace(GL_CCW); // Front faces are counter-clockwise by default

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
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // don't forget to enable shader before setting uniforms
        ourShader.use();

        glDisable(GL_DEPTH_TEST);
        //background plane rendered in otho 
        glm::mat4 orthoProjection = glm::ortho(0.0f, (float)screen.w, 0.0f, (float)screen.h, -10.0f, 10.0f);
        ourShader.setMat4("projection", orthoProjection);
        ourShader.setMat4("view", glm::mat4(1.0f));

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(screen.w / 2.0f, screen.h / 2.0f, 0.0f));
        model = glm::scale(model, glm::vec3(screen.w / 3.2f, screen.h / 1.8f, 1.0f));

        ourShader.setMat4("model", model);
        backgroundPlane.Draw(ourShader);
        glEnable(GL_DEPTH_TEST);

        // render the ball with perspective projecion
        glm::mat4 perspectiveProjection = glm::perspective(/*glm::radians(camera.Zoom)*/ FOV, (float)screen.w / (float)screen.h, 0.1f, 100.0f);
        //glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 view = glm::lookAt(CAMERA_POS, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        ourShader.setMat4("projection", perspectiveProjection);
        ourShader.setMat4("view", view);

        //drawing the firts ingredient in the list 
        ourShader.setMat4("model", active->GetModelMatrix());
        active->Move();
        active->Draw(ourShader);

        if (keys.PressedAndReleased(GLFW_MOUSE_BUTTON_LEFT)) {
            if (active->hit(mousePos, perspectiveProjection, view)) {
                ingredients.pop_front();
                if (ingredients.empty())
                    break;
                active = &ingredients[0];
                active->AddVelocity(active->getDirectionToCenter() * 5.0f);
                active->updateTime();
            }
        }
                
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
        //update pressed keys
        keys.Update(window);
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    /*
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS && glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL)
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    */

    //Camera movement
    /*
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    */


    //fullscreen
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
    {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, GLFW_DONT_CARE);

        screen.w = mode->width;
        screen.h = mode->height;
    }
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
        screen.resetSize();
        glfwSetWindowMonitor(window, NULL, 100, 100, screen.w, screen.h, 0);
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.

    if (width * screen.ARH > height * screen.ARW)
    {
        int viewport_width = (int)((double)height * screen.ARW / screen.ARH);
        int padding = (width - viewport_width) / 2;
        glViewport(0 + padding, 0, viewport_width, height);
        //update scren values
        screen.w = viewport_width;
        screen.paddingW = padding;
        screen.h = height;
        screen.paddingH = 0;
    }
    else
    {
        int viewport_height = (int)((double)width * screen.ARH / screen.ARW );
        int padding = (height - viewport_height) / 2;
        glViewport(0, 0 + padding, width, viewport_height);
        //update scren values
        screen.w = width;
        screen.paddingW = 0;
        screen.h = viewport_height;
        screen.paddingH = padding;
    }
}

float clampToUnitRange(float value) {
    if (value < 0) return 0;
    if (value > 1) return 1;
    return value;
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    mousePos = glm::vec2(xpos, ypos);

    float xval = clampToUnitRange((xpos - screen.paddingW) / screen.w);
    float yval = clampToUnitRange((ypos - screen.paddingH) / screen.h);

    //std::cout << ptr->hit(glm::vec2(xpos, ypos), *p, *v) << std::endl;

    //printf("%f | %f\n", xval, yval);

/*
    //CAMERA MOVEMENT FUNCTION
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

    if(glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
        camera.ProcessMouseMovement(xoffset, yoffset);
*/
   
    
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
}
