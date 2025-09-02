#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <random>
#include <limits>
#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "shader.h"
#include "camera.h"
#include "model.h"
#include "ingredient.h"
#include "vecplus.h"
#include "screen.h"
#include "keys.h"
#include "ScoreManager.h"
#include "TextRenderer.h"
#include "focusbox.h"
#include "button.h"
#include "Effects.h"
#include "GameState.h"

// -----------------------------------------------------------------------------
// Prototipi
// -----------------------------------------------------------------------------
void OnFramebufferSize(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window, FocusBox& focusBox);
void character_callback(GLFWwindow* window, unsigned int codepoint);
bool customWindowShouldClose(GLFWwindow* window);
void SpawnRandomIngredient();

// -----------------------------------------------------------------------------
// Stato globale (ordinato: prima Screen, poi variabili che lo usano)
// -----------------------------------------------------------------------------
Screen screen = Screen::S16_9();

float lastX = screen.w / 2.0f;
float lastY = screen.h / 2.0f;
bool firstMouse = true;

Camera camera(CAMERA_POS);

float focusSpeed = 0.3f;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// spawn
float spawnInterval = 2.0f;
float spawnTimer = 0.0f;

TextRenderer textRenderer;

bool endGame = false;
std::string inputName = "";
bool isTypingName = false;

bool fullscreen = false;

glm::vec2 mousePos = glm::vec2(0.0f);
glm::vec2 mouseScreenPos = glm::vec2(0.0f);

Keys keys;

std::random_device rd;
std::mt19937 gen(rd());

static ScoreManager scoreManager(3);

// effetti taglio
std::vector<SlashEffect> activeCuts;

// mouse trail
std::vector<glm::vec2> mouseTrail;
bool isDragging = false;

unsigned int trailVAO = 0, trailVBO = 0;
Shader* trailShader = nullptr;

// gioco
GameState gameState = GameState::MENU;
std::vector<Ingredient> ingredients;

// -----------------------------------------------------------------------------
// Funzioni di supporto
// -----------------------------------------------------------------------------
bool customWindowShouldClose(GLFWwindow* window) {
    return glfwWindowShouldClose(window);
}

void SpawnRandomIngredient() {
    std::vector<std::string> allIngredients = {
        "resources/ingredients/pumpkin/pumpkin.obj",
        "resources/ingredients/tomato/tomato.obj",
        "resources/ingredients/butter/butter.obj",
         "resources/ingredients/chocolate/chocolate.obj",
         "resources/ingredients/flour/flour.obj",
         "resources/ingredients/eggs/eggs.obj",
         "resources/ingredients/honey/honey.obj",
         "resources/ingredients/lemon/lemon.obj",
         "resources/ingredients/milk/milk.obj",
         "resources/ingredients/jam/jam.obj",
         "resources/ingredients/vanilla/vanilla.obj",
         "resources/ingredients/apple/apple.obj",
         "resources/ingredients/strawberry/strawberry.obj",
         "resources/ingredients/lem_marcio/lim_marcio.obj",
         "resources/ingredients/apple_gold/mela_oro.obj",
          "resources/ingredients/choc_marcio/cioc_marcio.obj",
         "resources/ingredients/strawb_gold/fragola_oro.obj",

         // altri ingredienti qui
    };

    int index = rand() % allIngredients.size();
    glm::vec2 spawn = Ingredient::RandomSpawnPoint();

    bool spawnBomb = (rand() % 100) < 20; // 20% bomba
    std::string path = spawnBomb
        ? "resources/ingredients/bomb/bomb.obj"
        : allIngredients[rand() % allIngredients.size()];


    // Direzione verso l'alto e centro, con piccola deviazione casuale
    glm::vec2 target(screen.w * 0.5f, screen.h * 0.7f); // centro alto
    glm::vec2 dir = glm::normalize(target - spawn);


    // ±12° per ridurre lo sbandamento laterale
    float angleOffset = ((rand() % 24) - 12) * 0.01745f;
    dir = RotateVec2(dir, angleOffset);

    // spinta un po' più “verticale/arcata”
    float speed = 600.0f + static_cast<float>(rand() % 100); // 600–699

    float scale = 30.0f;

    Ingredient item(path.c_str(), spawn, scale, spawnBomb);
    item.SetVelocity(dir * speed);
    item.updateTime();
    ingredients.push_back(item);


}

void character_callback(GLFWwindow* window, unsigned int codepoint) {
    if (gameState == GameState::NAME_INPUT) {
        if (codepoint == GLFW_KEY_BACKSPACE && !inputName.empty()) {
            inputName.pop_back();
        } else if (inputName.size() < 12 && codepoint >= 32 && codepoint <= 126) {
            inputName += static_cast<char>(codepoint);
        }
    }
}

// -----------------------------------------------------------------------------
// main
// -----------------------------------------------------------------------------
int main() {
    // glfw: init
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // window
    GLFWwindow* window = glfwCreateWindow(screen.w, screen.h, "Model_Loader", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    std::ifstream shaderCheck("text.vs");
    if (!shaderCheck.is_open()) {
        std::cerr << "text.vs non trovato nella working directory\n";
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, OnFramebufferSize);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetCharCallback(window, character_callback);

    // glad
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD\n";
        return -1;
    }

    // line shader per scie
    trailShader = new Shader("mouseTrail.vs", "mouseTrail.fs");
    glGenVertexArrays(1, &trailVAO);
    glGenBuffers(1, &trailVBO);
    glBindVertexArray(trailVAO);
    glBindBuffer(GL_ARRAY_BUFFER, trailVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * 500, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // focus box
    FocusBox focusBox(glm::vec2(200.0f, 120.0f));
    focusBox.SetCenter(glm::vec2(0.5f, 0.5f)); // in 0..1

    // testo
    Shader textShader("text.vs", "text.fs");
    if (!textRenderer.Load("resources/arial.ttf", 48)) {
        std::cerr << "Errore caricamento font!\n";
        return -1;
    }
    textShader.use();
    glm::mat4 textProj = glm::ortho(0.0f, (float)screen.w, 0.0f, (float)screen.h);
    textShader.setMat4("projection", textProj);

    // opengl state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // shaders / modelli
    Shader ourShader("shader.vs", "shader.fs");
    Shader focusShader("focusbox.vs", "focusbox.fs");
    Shader blurShader("shader.vs", "shader_blur.fs");
    Shader objBlurShader("shader.vs", "shader_blur_objs.fs");

    Model backgroundPlane("resources/background/background.obj");
    Model playButtonModel("resources/buttons/playButton.obj");
    Model scoresButtonModel("resources/buttons/scoresButton.obj");
    Model infoButtonModel("resources/buttons/infoButton.obj");
    Model lifeIcon("resources/ingredients/heart/heart.obj");

    // bottoni (posizioni in 0..1)
    glm::vec2 baseSize = glm::vec2(100.0f, 50.0f);
    float playYScale = 1.0f, scoresYScale = 1.0f, infoYScale = 1.0f;

    Button playButton{  glm::vec2(0.5f, 0.75f), baseSize, playYScale,   "PLAY"   };
    Button scoresButton{glm::vec2(0.5f, 0.50f), baseSize, scoresYScale, "SCORES" };
    Button infoButton{  glm::vec2(0.5f, 0.25f), baseSize, infoYScale,   "INFO"   };

    // loop
    while (!customWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window, focusBox);

        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (gameState == GameState::MENU) {
            ourShader.use();
            glDisable(GL_DEPTH_TEST);

            glm::mat4 orthoProj = glm::ortho(0.0f, (float)screen.w, 0.0f, (float)screen.h, -10.0f, 10.0f);
            ourShader.setMat4("projection", orthoProj);
            ourShader.setMat4("view", glm::mat4(1.0f));

            // background
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(screen.w / 2.0f, screen.h / 2.0f, 0.0f));
            model = glm::scale(model, glm::vec3(screen.w / 3.2f, screen.h / 1.8f, 1.0f));
            ourShader.setMat4("model", model);
            ourShader.setBool("hasTexture", true);
            ourShader.setVec3("diffuseColor", glm::vec3(1.0f));
            backgroundPlane.Draw(ourShader);

            // bottoni
            ourShader.setBool("hasTexture", false);
            playButton.Draw(ourShader, playButtonModel);
            scoresButton.Draw(ourShader, scoresButtonModel);
            infoButton.Draw(ourShader, infoButtonModel);

            if (keys.PressedAndReleased(GLFW_MOUSE_BUTTON_LEFT) && playButton.isClicked(mousePos)) {
                ingredients.clear();
                activeCuts.clear();
                spawnTimer = 0.0f;
                scoreManager.reset(3);           // reset punteggio e vite
                gameState = GameState::PLAYING;
                glEnable(GL_DEPTH_TEST);
                continue;
            }
            if (keys.PressedAndReleased(GLFW_MOUSE_BUTTON_LEFT) && scoresButton.isClicked(mousePos)) {
                gameState = GameState::SCORES;
            }
            if (keys.PressedAndReleased(GLFW_MOUSE_BUTTON_LEFT) && infoButton.isClicked(mousePos)) {
                gameState = GameState::INFO;
            }
        }
        else if (gameState == GameState::PLAYING) {
            const double now = glfwGetTime();

            blurShader.use();
            blurShader.setVec2("uTexelSize", glm::vec2(1.0f / screen.w, 1.0f / screen.h));

            // rettangolo non blur
            glm::vec2 rectSize = focusBox.getScaledSize() * 2.0f;
            glm::vec2 rectPos  = focusBox.GetCenter();
            rectSize.x /= screen.w; rectSize.y /= screen.h;
            rectPos.y = 1 - rectPos.y;

            float rectMinX = glm::max(0.0f, rectPos.x - rectSize.x * 0.5f);
            float rectMinY = glm::max(0.0f, rectPos.y - rectSize.y * 0.5f);
            float rectMaxX = glm::min(1.0f, rectPos.x + rectSize.x * 0.5f);
            float rectMaxY = glm::min(1.0f, rectPos.y + rectSize.y * 0.5f);
            blurShader.setVec2("rectMin", glm::vec2(rectMinX, rectMinY));
            blurShader.setVec2("rectMax", glm::vec2(rectMaxX, rectMaxY));

            glDisable(GL_DEPTH_TEST);

            // background (blur shader)
            glm::mat4 orthoProj = glm::ortho(0.0f, (float)screen.w, 0.0f, (float)screen.h, -10.0f, 10.0f);
            blurShader.setMat4("projection", orthoProj);
            blurShader.setMat4("view", glm::mat4(1.0f));

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(screen.w / 2.0f, screen.h / 2.0f, 0.0f));
            model = glm::scale(model, glm::vec3(screen.w / 3.2f, screen.h / 1.8f, 1.0f));
            blurShader.setMat4("model", model);
            blurShader.setBool("hasTexture", true);
            blurShader.setVec3("diffuseColor", glm::vec3(1.0f));
            backgroundPlane.Draw(blurShader);


            // Ingredients
            //---------------
            glm::mat4 ingProj = glm::ortho(0.0f, (float)screen.w, 0.0f, (float)screen.h, -10.0f, 10.0f);
            glm::mat4 ingView(1.0f);
            glm::mat4 perspectiveProj = glm::perspective(glm::radians(camera.Zoom), (float)screen.w / (float)screen.h, 0.1f, 100.0f);
            glm::mat4 view = camera.GetViewMatrix();
            model = glm::scale(model, glm::vec3(20.0f));  // per test visibilità

            spawnTimer += deltaTime;
            if (spawnTimer >= spawnInterval) {
                SpawnRandomIngredient();
                spawnTimer = 0.0f;
            }



            if (!ingredients.empty()) {


                glEnable(GL_DEPTH_TEST);
                ourShader.use();
                ourShader.setVec3("diffuseColor", glm::vec3(1.0f));  // colore fallback bianco
                ourShader.setBool("hasTexture", true);              // o false se vuoi colore puro


                glm::mat4 perspectiveProj = glm::perspective(glm::radians(camera.Zoom), (float)screen.w / (float)screen.h, 0.1f, 100.0f);
                glm::mat4 view = camera.GetViewMatrix();

                // Taglio con mouse trail
                if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                    if (!isDragging) {
                        isDragging = true;
                        mouseTrail.clear();
                    }
                }
                else {


                    if (isDragging) {
                        isDragging = false;
                        if (!mouseTrail.empty() && camera.Zoom != 0.0f) {
                            // Se c'è una scia, processala
                            scoreManager.processSlash(
                                mouseTrail, perspectiveProj, view, focusBox,
                                ingredients, activeCuts,
                                screen,
                                now,
                                gameState
                            );
                        }

                        mouseTrail.clear();
                    }
                }

                // Aggiorna e disegna tutti gli ingredienti
                objBlurShader.use();
                objBlurShader.setVec2("uInvViewport", glm::vec2(1.0f / screen.w, 1.0f / screen.h));
                objBlurShader.setMat4("projection", ingProj);
                objBlurShader.setMat4("view", ingView);
                objBlurShader.setVec2("rectMin", glm::vec2(rectMinX, rectMinY));
                objBlurShader.setVec2("rectMax", glm::vec2(rectMaxX, rectMaxY));

                glDisable(GL_DEPTH_TEST);
                for (auto& ing : ingredients) {
                    ing.Move();
                    //texure height and width foer uTextelSize
                    Texture* t = ing.getFirstTexture();
                    if (t != nullptr) {
                        int tW = t->w;
                        int tH = t->h;
                        objBlurShader.setVec2("uTexelSize", glm::vec2(1.0f / tW, 1.0f / tH));
                    }
                    objBlurShader.setMat4("model", ing.GetModelMatrix());
                    objBlurShader.setBool("hasTexture", true);
                    objBlurShader.setVec3("diffuseColor", glm::vec3(1.0f));
                    ing.Draw(objBlurShader);

                }
            }

            glDisable(GL_DEPTH_TEST);

            // UI: vite
            ourShader.use();
            glm::mat4 uiProj = glm::ortho(0.0f, (float)screen.w, 0.0f, (float)screen.h, -10.0f, 10.0f);
            ourShader.setMat4("projection", uiProj);
            ourShader.setMat4("view", glm::mat4(1.0f));
            ourShader.setBool("hasTexture", true);
            ourShader.setVec3("diffuseColor", glm::vec3(1));

            const float padX = 60.0f, padY = 60.0f, iconSize = 28.0f, step = 80.0f;
            for (int i = 0; i < scoreManager.getLives(); ++i) {
                float x = padX + i * (iconSize + step);
                float y = screen.h - padY;
                glm::mat4 m(1.0f);
                m = glm::translate(m, glm::vec3(x, y, 0.0f));
                m = glm::scale(m, glm::vec3(iconSize, iconSize, 1.0f));
                ourShader.setMat4("model", m);
                lifeIcon.Draw(ourShader);
            }

            // Score UI
            textShader.use();
            textShader.setMat4("projection", glm::ortho(0.0f, (float)screen.w, 0.0f, (float)screen.h));
            std::string scoreText = "Score: " + std::to_string(scoreManager.getScore());
            textRenderer.DrawText(textShader, scoreText, screen.w - 250.0f, screen.h - 60.0f, 1.0f, glm::vec3(1.0f, 1.0f, 0.4f));

            // trail durante drag + processo live
            if (isDragging && mouseTrail.size() >= 2) {
                trailShader->use();
                glm::mat4 proj = glm::ortho(0.0f, (float)screen.w, 0.0f, (float)screen.h);
                trailShader->setMat4("projection", proj);
                trailShader->setVec3("trailColor", glm::vec3(1.0f, 1.0f, 0.4f));

                std::vector<float> trailData;
                trailData.reserve(mouseTrail.size() * 2);
                for (const auto& pt : mouseTrail) { trailData.push_back(pt.x); trailData.push_back(pt.y); }
                glBindBuffer(GL_ARRAY_BUFFER, trailVBO);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * trailData.size(), trailData.data());
                glBindVertexArray(trailVAO);
                glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)mouseTrail.size());
                glBindVertexArray(0);

                scoreManager.processSlash(
                    mouseTrail, perspectiveProj, view, focusBox,
                    ingredients, activeCuts,
                    screen,
                    now,
                    gameState
                );
            }

            // disegna effetti taglio
            glLineWidth(4.0f);
            trailShader->use();
            trailShader->setVec3("trailColor", glm::vec3(1.0f, 1.0f, 0.4f));
            trailShader->setMat4("projection", glm::ortho(0.0f, (float)screen.w, 0.0f, (float)screen.h));

            std::vector<float> cutData;
            for (auto it = activeCuts.begin(); it != activeCuts.end();) {
                double age = now - it->timestamp;
                if (age > 0.2) { it = activeCuts.erase(it); continue; }
                cutData.push_back(it->start.x); cutData.push_back(it->start.y);
                cutData.push_back(it->end.x);   cutData.push_back(it->end.y);
                ++it;
            }
            if (!cutData.empty()) {
                glBindBuffer(GL_ARRAY_BUFFER, trailVBO);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * cutData.size(), cutData.data());
                glBindVertexArray(trailVAO);
                glDrawArrays(GL_LINES, 0, (GLsizei)(cutData.size() / 2));
                glBindVertexArray(0);
            }

            glEnable(GL_DEPTH_TEST);
        }
        else if (gameState == GameState::SCORES) {
            auto scores = ScoreManager::LoadScores("score.txt");

            textShader.use();
            glm::mat4 projection = glm::ortho(0.0f, (float)screen.w, 0.0f, (float)screen.h);
            textShader.setMat4("projection", projection);

            float y = screen.h - 100.0f;
            float x = 100.0f;

            textRenderer.DrawText(textShader, "PUNTEGGI", x, y, 1.5f, glm::vec3(1.0f, 0.8f, 0.0f));
            y -= 80.0f;

            if (scores.empty()) {
                textRenderer.DrawText(textShader, "Nessun punteggio salvato.", x, y, 1.0f, glm::vec3(1.0f));
            } else {
                int index = 1;
                for (const auto& entry : scores) {
                    std::string line = std::to_string(index++) + ". " + entry.name + ": " + std::to_string(entry.score);
                    textRenderer.DrawText(textShader, line, x, y, 1.0f, glm::vec3(1.0f));
                    y -= 60.0f;
                }
            }
            if (textRenderer.Characters.empty()) {
                std::cerr << "Font non inizializzato!\n";
                continue;
            }
        }
        else if (gameState == GameState::NAME_INPUT) {
            glDisable(GL_DEPTH_TEST);
            textShader.use();

            textRenderer.DrawText(textShader, "INSERISCI NOME:", screen.w / 3, screen.h / 2 + 50, 1.0f, glm::vec3(1.0f));
            textRenderer.DrawText(textShader, inputName + "_",  screen.w / 3, screen.h / 2,       1.0f, glm::vec3(0.5f, 1.0f, 0.5f));

            // BACKSPACE con delay
            if (glfwGetKey(window, GLFW_KEY_BACKSPACE) == GLFW_PRESS && !inputName.empty()) {
                static double lastDeleteTime = 0;
                double currentTime = glfwGetTime();
                if (currentTime - lastDeleteTime > 0.15) {
                    inputName.pop_back();
                    lastDeleteTime = currentTime;
                }
            }

            // ENTER (normale + tastierino)
            if ((glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS ||
                 glfwGetKey(window, GLFW_KEY_KP_ENTER) == GLFW_PRESS)) {

                if (inputName.empty()) inputName = "Anonimo";
                ScoreManager::SaveScore(inputName, scoreManager.getScore());
                gameState = GameState::SCORES;
            }
        }
        else if (gameState == GameState::PAUSED) {
            glDisable(GL_DEPTH_TEST);
            textShader.use();
            glm::mat4 projection = glm::ortho(0.0f, (float)screen.w, 0.0f, (float)screen.h);
            textShader.setMat4("projection", projection);
            textRenderer.DrawText(textShader, "GAME PAUSED",           screen.w / 2 - 150.0f, screen.h / 2,         1.5f, glm::vec3(1.0f, 0.5f, 0.0f));
            textRenderer.DrawText(textShader, "Press P to RESUME",     screen.w / 2 - 200.0f, screen.h / 2 - 100.0f, 1.0f, glm::vec3(1.0f));
            textRenderer.DrawText(textShader, "Press ESC to EXIT MENU",screen.w / 2 - 220.0f, screen.h / 2 - 150.0f, 1.0f, glm::vec3(1.0f));
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
        keys.Update(window);
    }

    if (trailShader) delete trailShader;
    glDeleteVertexArrays(1, &trailVAO);
    glDeleteBuffers(1, &trailVBO);

    glfwTerminate();
    return 0;
}

// -----------------------------------------------------------------------------
// Input e callback
// -----------------------------------------------------------------------------
void processInput(GLFWwindow* window, FocusBox& focusBox) {
    float currentSpeed = focusSpeed * deltaTime;

    // boost con SPACE
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) currentSpeed *= 8.0f;

    if (gameState == GameState::PLAYING) {
        float s = currentSpeed;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) focusBox.Move({ 0.0f,  s });
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) focusBox.Move({ 0.0f, -s });
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) focusBox.Move({ -s,   0.0f });
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) focusBox.Move({  s,   0.0f });

        if (glfwGetKey(window, GLFW_KEY_UP)    == GLFW_PRESS) focusBox.Move({ 0.0f,  s });
        if (glfwGetKey(window, GLFW_KEY_DOWN)  == GLFW_PRESS) focusBox.Move({ 0.0f, -s });
        if (glfwGetKey(window, GLFW_KEY_LEFT)  == GLFW_PRESS) focusBox.Move({ -s,   0.0f });
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) focusBox.Move({  s,   0.0f });
    }

    // fullscreen toggle
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !keys.keyLock[GLFW_KEY_F] && fullscreen == false) {
        fullscreen = true;
        keys.keyLock[GLFW_KEY_F] = true;
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, GLFW_DONT_CARE);
    }
    else if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !keys.keyLock[GLFW_KEY_F] && fullscreen == true) {
        keys.keyLock[GLFW_KEY_F] = true;
        fullscreen = false;
        screen.resetSize();
        glfwSetWindowMonitor(window, NULL, 100, 100, screen.w, screen.h, 0);
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE) {
        keys.keyLock[GLFW_KEY_F] = false;
    }

    // pausa P
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !keys.keyLock[GLFW_KEY_P]) {
        keys.keyLock[GLFW_KEY_P] = true;
        if (gameState == GameState::PLAYING)      gameState = GameState::PAUSED;
        else if (gameState == GameState::PAUSED)  gameState = GameState::PLAYING;
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE) {
        keys.keyLock[GLFW_KEY_P] = false;
    }

    // ESC
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS && !keys.keyLock[GLFW_KEY_ESCAPE]) {
        keys.keyLock[GLFW_KEY_ESCAPE] = true;

        if (gameState == GameState::PLAYING) {
            endGame = true;
            gameState = GameState::NAME_INPUT;
        }
        else if (gameState == GameState::SCORES || gameState == GameState::INFO || gameState == GameState::PAUSED) {
            gameState = GameState::MENU;
        }
        else if (gameState == GameState::MENU) {
            ingredients.clear();
            endGame = false;
            glfwSetWindowShouldClose(window, true);
        }
    }
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_RELEASE) {
        keys.keyLock[GLFW_KEY_ESCAPE] = false;
    }
}

void OnFramebufferSize(GLFWwindow* window, int width, int height) {
    if (width * screen.ARH > height * screen.ARW) {
        int viewport_width = (int)((double)height * screen.ARW / screen.ARH);
        int padding = (width - viewport_width) / 2;
        glViewport(padding, 0, viewport_width, height);
        screen.w = viewport_width; screen.paddingW = padding;
        screen.h = height;         screen.paddingH = 0;
    } else {
        int viewport_height = (int)((double)width * screen.ARH / screen.ARW);
        int padding = (height - viewport_height) / 2;
        glViewport(0, padding, width, viewport_height);
        screen.w = width;          screen.paddingW = 0;
        screen.h = viewport_height;screen.paddingH = padding;
    }
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = (float)xposIn;
    float ypos = (float)yposIn;

    float orthoX = xpos - screen.paddingW;
    float orthoY = screen.h - (ypos - screen.paddingH);
    mousePos = glm::vec2(orthoX, orthoY);
    mouseScreenPos = glm::vec2(xpos, ypos);

    if (firstMouse) {
        lastX = xpos; lastY = ypos; firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos; lastY = ypos;

    if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
        camera.ProcessMouseMovement(xoffset, yoffset);

    if (isDragging) {
        if (mouseTrail.empty() || glm::distance(mouseTrail.back(), mousePos) > 3.0f) {
            mouseTrail.push_back(mousePos);
            if (mouseTrail.size() > 25) mouseTrail.erase(mouseTrail.begin());
        }
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    (void)window; (void)xoffset; (void)yoffset;
}
