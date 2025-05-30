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

enum class GameState {
    MENU,
    PLAYING,
    SCORES,
    INFO
};

GameState gameState = GameState::MENU;
std::deque<Ingredient> ingredients;
Ingredient* active = nullptr;

struct Button {
    glm::vec2 pos;
    glm::vec2 size;
    float yScale;
    std::string label;

    bool isClicked(glm::vec2 mouse) const {
        float halfHeight = size.y * yScale / 2.0f;
        return mouse.x >= pos.x - size.x / 2 && mouse.x <= pos.x + size.x / 2 &&
            mouse.y >= pos.y - halfHeight && mouse.y <= pos.y + halfHeight;
    }

    void Draw(Shader& shader, Model& model) {
        glm::mat4 mat = glm::mat4(1.0f);
        mat = glm::translate(mat, glm::vec3(pos, 0.0f));
        mat = glm::scale(mat, glm::vec3(size.x, size.y * yScale, 1.0f));
        shader.setMat4("model", mat);
        model.Draw(shader);
    }
};


bool customWindowShouldClose(GLFWwindow* window) {
    return glfwWindowShouldClose(window) || (gameState == GameState::PLAYING && ingredients.empty());
}


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
    Model playButtonModel("resources/buttons/playButton.obj");
    Model scoresButtonModel("resources/buttons/scoresButton.obj");
    Model infoButtonModel("resources/buttons/infoButton.obj");

    glm::vec2 baseSize = glm::vec2(100.0f, 50.0f);
    float spacing = 100.0f;

    float playYScale = 1.0f;
    float scoresYScale = 1.0f;
    float infoYScale = 1.0f;

    // Calcolo altezza totale
    float totalHeight =
        baseSize.y * playYScale +
        baseSize.y * scoresYScale +
        baseSize.y * infoYScale +
        2 * spacing;

    float topY = screen.h / 2.0f + totalHeight / 2.0f - baseSize.y * playYScale / 2.0f;

    Button playButton{
        glm::vec2(screen.w / 2.0f, topY),
        baseSize,
        playYScale,
        "PLAY"
    };

    Button scoresButton{
        glm::vec2(screen.w / 2.0f, topY - (baseSize.y * playYScale + spacing)),
        baseSize,
        scoresYScale,
        "SCORES"
    };

    Button infoButton{
        glm::vec2(screen.w / 2.0f, topY - (baseSize.y * playYScale + spacing) - (baseSize.y * scoresYScale + spacing)),
        baseSize,
        infoYScale,
        "INFO"
    };


    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glDisable(GL_CULL_FACE);
    //glCullFace(GL_BACK); // Cull back faces
    //glFrontFace(GL_CCW); // Front faces are counter-clockwise by default

    // render loop
    // -----------
    while (!customWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ourShader.use();

        if (gameState == GameState::MENU) {
            glDisable(GL_DEPTH_TEST);

            glm::mat4 orthoProj = glm::ortho(0.0f, (float)screen.w, 0.0f, (float)screen.h, -10.0f, 10.0f);
            ourShader.setMat4("projection", orthoProj);
            ourShader.setMat4("view", glm::mat4(1.0f));

            // Background
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(screen.w / 2.0f, screen.h / 2.0f, 0.0f));
            model = glm::scale(model, glm::vec3(screen.w / 3.2f, screen.h / 1.8f, 1.0f));
            ourShader.setMat4("model", model);
            ourShader.setBool("hasTexture", true);  // background ha texture
            ourShader.setVec3("diffuseColor", glm::vec3(1.0f)); // fallback nel caso
            backgroundPlane.Draw(ourShader);

            // Play button (senza texture, colore da MTL o fisso)
            ourShader.setBool("hasTexture", false);
            playButton.Draw(ourShader, playButtonModel);

            // Scores button
            ourShader.setBool("hasTexture", false);
            scoresButton.Draw(ourShader, scoresButtonModel);

            // Info button
            ourShader.setBool("hasTexture", false);
            infoButton.Draw(ourShader, infoButtonModel);


            if (keys.PressedAndReleased(GLFW_MOUSE_BUTTON_LEFT) && playButton.isClicked(mousePos)) {
                ingredients.clear();
                for (int i = 0; i < 4; ++i)
                    ingredients.push_back(Ingredient("resources/ball/ball.obj", Ingredient::RandomSpawnPoint()));
                active = &ingredients[0];
                active->AddVelocity(active->getDirectionToCenter() * 5.0f);
                active->updateTime();
                gameState = GameState::PLAYING;
            }
            if (keys.PressedAndReleased(GLFW_MOUSE_BUTTON_LEFT) && scoresButton.isClicked(mousePos)) {
                std::cout << "Scores clicked\n";
                gameState = GameState::SCORES;
            }

            if (keys.PressedAndReleased(GLFW_MOUSE_BUTTON_LEFT) && infoButton.isClicked(mousePos)) {
                std::cout << "Info clicked\n";
                gameState = GameState::INFO;
            }


        }
        else if (gameState == GameState::PLAYING) {
            glm::mat4 perspectiveProj = glm::perspective(FOV, (float)screen.w / (float)screen.h, 0.1f, 100.0f);
            glm::mat4 view = glm::lookAt(CAMERA_POS, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

            ourShader.setMat4("projection", perspectiveProj);
            ourShader.setMat4("view", view);

            if (!ingredients.empty()) {
                ourShader.setMat4("model", active->GetModelMatrix());
                ourShader.setBool("hasTexture", true);  // ingredienti con texture
                ourShader.setVec3("diffuseColor", glm::vec3(1.0f)); // fallback nel caso
                active->Move();
                active->Draw(ourShader);

                if (keys.PressedAndReleased(GLFW_MOUSE_BUTTON_LEFT) &&
                    active->hit(mousePos, perspectiveProj, view)) {
                    ingredients.pop_front();
                    if (!ingredients.empty()) {
                        active = &ingredients[0];
                        active->AddVelocity(active->getDirectionToCenter() * 5.0f);
                        active->updateTime();
                    }
                }
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
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

    // Converti coordinate mouse in coordinate ortografiche usate dal bottone
    float orthoX = xpos - screen.paddingW;
    float orthoY = screen.h - (ypos - screen.paddingH);  // inverti asse Y

    mousePos = glm::vec2(orthoX, orthoY);
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
