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
#include <unordered_map>
#include <memory>

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
void SpawnRandomIngredientWithId(std::vector<Ingredient>& ingredients,
                                 std::vector<std::string>& ingredientIds);


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

float scoreScrollOffset = 0.0f;

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

void SpawnRandomIngredientWithId(std::vector<Ingredient>& ingredients,
                                 std::vector<std::string>& ingredientIds)
{
    struct SpawnDef {
        const char* path;
        const char* id;
        float scale = 30.0f;
        bool isBomb = false;
    };

    // Pool non-bomba (mappa ogni OBJ a un ID "logico")
    static const std::vector<SpawnDef> kNonBomb = {
        {"resources/ingredients/pumpkin/pumpkin.obj",        "pumpkin"},
        {"resources/ingredients/tomato/tomato.obj",          "tomato"},
        {"resources/ingredients/butter/butter.obj",          "butter"},
        {"resources/ingredients/chocolate/chocolate.obj",    "chocolate"},
        {"resources/ingredients/flour/flour.obj",            "flour"},
        {"resources/ingredients/eggs/eggs.obj",              "eggs"},
        {"resources/ingredients/honey/honey.obj",            "honey"},
        {"resources/ingredients/lemon/lemon.obj",            "lemon"},
        {"resources/ingredients/milk/milk.obj",              "milk"},
        {"resources/ingredients/jam/jam.obj",                "jam"},
        {"resources/ingredients/vanilla/vanilla.obj",        "vanilla"},
        {"resources/ingredients/apple/apple.obj",            "apple"},
        {"resources/ingredients/strawberry/strawberry.obj",  "strawberry"},
        {"resources/ingredients/lem_marcio/lim_marcio.obj",  "lim_marcio"},
        {"resources/ingredients/apple_gold/mela_oro.obj",    "mela_oro"},
        {"resources/ingredients/choc_marcio/cioc_marcio.obj","cioc_marcio"},
        {"resources/ingredients/strawb_gold/fragola_oro.obj","fragola_oro"},
    };

    static const SpawnDef kBomb = {
        "resources/ingredients/bomb/bomb.obj", "bomb", 30.0f, true
    };

	static const SpawnDef kHeart = {
		"resources/ingredients/heart/heart.obj","heart", 28.0f, false
	};

    // --- Scelta casuale tipo spawn ---
    std::uniform_int_distribution<int> perc(0, 99);
	const bool spawnHeart = (perc(gen) < 10);  // ~8% cuore
    bool spawnBomb = (perc(gen) < 20);  // 20% bombe

    SpawnDef chosen;
    if (spawnHeart) {
        chosen = kHeart;
    } else if (spawnBomb) {
        chosen = kBomb;
    } else {
        std::uniform_int_distribution<size_t> pick(0, kNonBomb.size() - 1);
        chosen = kNonBomb[pick(gen)];
    }

    // --- Spawn e fisica iniziale (coerente con la tua funzione) ---
    glm::vec2 spawn = Ingredient::RandomSpawnPoint();

    glm::vec2 target(screen.w * 0.5f, screen.h * 0.7f);    // verso alto-centro
    glm::vec2 dir = glm::normalize(target - spawn);

    std::uniform_real_distribution<float> angleDeg(-12.0f, 12.0f);
    dir = RotateVec2(dir, glm::radians(angleDeg(gen)));

    std::uniform_real_distribution<float> speedJit(600.0f, 700.0f);
    float speed = speedJit(gen);

    Ingredient item(chosen.path, spawn, chosen.scale, chosen.isBomb);
    item.SetVelocity(dir * speed);
    item.updateTime();

    ingredients.emplace_back(std::move(item));
    ingredientIds.emplace_back(chosen.id);   // <-- allinea l’ID al nuovo ingrediente
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
// Crea i punti di un contorno rettangolare con angoli smussati
static void buildRoundedRectOutline(float x, float y, float w, float h,
    float r, int segs, std::vector<glm::vec2>& out)
{
    out.clear();
    r = glm::clamp(r, 0.0f, 0.5f * glm::min(w, h));

    auto arc = [&](float cx, float cy, float a0, float a1) {
        for (int i = 0; i <= segs; ++i) {
            float t = a0 + (a1 - a0) * (float)i / (float)segs;
            out.emplace_back(cx + r * cosf(t), cy + r * sinf(t));
        }
        };

    // coordinate schermo: (0,0) in basso a sinistra, Y verso l'alto
    // angoli: 0 = +X, pi/2 = +Y, ...
    // giriamo in senso antiorario partendo dal fondo-sx
    // bottom-left (pi -> 3pi/2)
    arc(x + r, y + r, glm::pi<float>(), 1.5f * glm::pi<float>());
    // bottom-right (3pi/2 -> 2pi)
    arc(x + w - r, y + r, 1.5f * glm::pi<float>(), 2.0f * glm::pi<float>());
    // top-right (0 -> pi/2)
    arc(x + w - r, y + h - r, 0.0f, 0.5f * glm::pi<float>());
    // top-left (pi/2 -> pi)
    arc(x + r, y + h - r, 0.5f * glm::pi<float>(), glm::pi<float>());
    // chiudi la strip tornando al primo punto
    out.push_back(out.front());
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

	// build and compile shaders
	// -------------------------
	Shader ourShader("shader.vs", "shader.fs");
	//shader per focus box
	Shader focusShader("focusbox.vs", "focusbox.fs");
	Shader blurShader("shader.vs", "shader_blur.fs");
	Shader objBlurShader("shader.vs", "shader_blur_objs.fs");
	// load models
	// -----------
	Model backgroundPlane("resources/background/table.obj");
	Model playButtonModel("resources/buttons/play.obj");
	Model scoresButtonModel("resources/buttons/scores.obj");
	Model infoButtonModel("resources/buttons/info.obj");
    Model lifeIcon("resources/ingredients/heart/heart.obj");
	Model parchment("resources/recipes/pergamena.obj");

	std::vector<Model> recipeModels;
	recipeModels.reserve(10);
	for (int i = 1; i <= 10; ++i) {
		recipeModels.emplace_back(("resources/recipes/ricetta" + std::to_string(i) + ".obj").c_str());
	}

	// Modelli "Livello X completato"
	std::vector<std::unique_ptr<Model>> levelModels;
	levelModels.reserve(10);
	for (int i = 1; i <= 10; ++i) {
		std::string p = "resources/levels/livello" + std::to_string(i) + ".obj";
		levelModels.emplace_back(std::make_unique<Model>(p.c_str()));
	}


	// durata anteprima ricetta
	constexpr double kRecipePreview = 10.0;  
	constexpr double kCongratsTime  = 5.0;

	// timestamp inizio stato RECIPE
	int currentLevel = 1;
	double recipeStartTime = 0.0;
	double congratsStartTime = 0.0;


	// vettore parallelo agli ingredienti per tenerne l'ID
	std::vector<std::string> ingredientIds;                 // <--- AGGIUNTO
	std::vector<std::string> recipeIds;

	std::vector<std::unordered_map<std::string,int>> levelRecipes = {
		{{"milk",1}, {"eggs",1}},                                  // 1
		{{"flour",2}, {"milk",1}, {"eggs",1}},                     // 2
		{{"tomato",3}},                                            // 3
		{{"apple",2}, {"strawberry",2}},                           // 4
		{{"vanilla",1}, {"honey",2}, {"milk",1}},                  // 5
		{{"chocolate",2}, {"milk",1}},                             // 6
		{{"butter",2}, {"flour",2}},                               // 7
		{{"lemon",3}},                                             // 8
		{{"apple",2}, {"eggs",1}},                                   // 9
		{{"pumpkin",2}, {"flour",2}, {"eggs",1}, {"milk",1}}       // 10
	};

	auto applyLevel = [&](int lvl){
		scoreManager.setRequiredRecipe(levelRecipes[lvl-1]);
		recipeIds.clear();
		for (auto &kv : levelRecipes[lvl-1]) recipeIds.push_back(kv.first);
	}; 

   
	glm::vec2 baseSize = glm::vec2(100.0f, 50.0f);
	float spacing = 100.0f;

	float playYScale = 2.0f;
	float scoresYScale = 2.0f;
	float infoYScale = 2.0f;

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

			//aggiorna hover
			playButton.UpdateHover(mousePos);
			scoresButton.UpdateHover(mousePos);
			infoButton.UpdateHover(mousePos);

            // bottoni
            ourShader.setBool("hasTexture", false);
            playButton.Draw(ourShader, playButtonModel);
            scoresButton.Draw(ourShader, scoresButtonModel);
            infoButton.Draw(ourShader, infoButtonModel);

            if (keys.PressedAndReleased(GLFW_MOUSE_BUTTON_LEFT) && playButton.isClicked(mousePos)) {
                ingredients.clear();
				ingredientIds.clear();
                activeCuts.clear();
                spawnTimer = 0.0f;
                scoreManager.reset(3);           // reset punteggio e vite

				currentLevel = 1;
				applyLevel(currentLevel);

                // Avvia stato RECIPE (3s)
				recipeStartTime = glfwGetTime();
				gameState = GameState::RECIPE;
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
		else if (gameState == GameState::RECIPE) {
			const double now = glfwGetTime();

			// -- background come in PLAYING --
			blurShader.use();
			blurShader.setVec2("uTexelSize", glm::vec2(1.0f / screen.w, 1.0f / screen.h));

			blurShader.setVec2("rectMin", glm::vec2(-1.0f, -1.0f));
    		blurShader.setVec2("rectMax", glm::vec2(-1.0f, -1.0f));
			// se usi il focus blur anche qui, ripeti il setup del rect (facoltativo)
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

			// -- mostra ricetta1.obj in ortho centrato --
			glDisable(GL_DEPTH_TEST);
			ourShader.use();
			ourShader.setMat4("projection", orthoProj);
			ourShader.setMat4("view", glm::mat4(1.0f));
			ourShader.setBool("hasTexture", true);
			ourShader.setVec3("diffuseColor", glm::vec3(1.0f));

			glm::mat4 rm(1.0f);
			rm = glm::translate(rm, glm::vec3(screen.w * 0.5f, screen.h * 0.5f, 0.0f));
			float s = std::min(screen.w, screen.h) * 0.18f;
			rm = glm::scale(rm, glm::vec3(s, s, 1.0f));
			ourShader.setMat4("model", rm);
			recipeModels[currentLevel-1].Draw(ourShader);

			// countdown in alto a destra
			textShader.use();
			textShader.setMat4("projection", orthoProj);
			int remaining = (int)std::ceil(kRecipePreview - (now - recipeStartTime));
			if (remaining < 0) remaining = 0;
			textRenderer.DrawText(textShader, "Time: " + std::to_string(remaining),
								screen.w - 220.0f, screen.h - 60.0f, 1.0f, glm::vec3(1.0f, 1.0f, 0.4f));

			// dopo 3s vai a PLAYING
			if (now - recipeStartTime >= kRecipePreview) {
				spawnTimer = 0.0f;              // così lo spawn parte “fresco”
				glEnable(GL_DEPTH_TEST);
				gameState = GameState::PLAYING;
				continue;                       // passa subito al frame PLAYING
			}
		}

        else if (gameState == GameState::PLAYING) {
			const double now = glfwGetTime();

			// --- Blur pass + focus rect (invariato) ---
			blurShader.use();
			blurShader.setVec2("uTexelSize", glm::vec2(1.0f / screen.w, 1.0f / screen.h));

			glm::vec2 rectSize = focusBox.getScaledSize() * 2.0f;
			glm::vec2 rectPos  = focusBox.GetCenter();
			rectSize.x /= screen.w; rectSize.y /= screen.h;
			rectPos.y = 1 - rectPos.y;
			float rectMinX = rectPos.x - rectSize.x / 2.0f; if (rectMinX < 0.0f) rectMinX = 0.0f;
			float rectMinY = rectPos.y - rectSize.y / 2.0f; if (rectMinY < 0.0f) rectMinY = 0.0f;
			float rectMaxX = rectPos.x + rectSize.x / 2.0f; if (rectMaxX > 1.0f) rectMaxX = 1.0f;
			float rectMaxY = rectPos.y + rectSize.y / 2.0f; if (rectMaxY > 1.0f) rectMaxY = 1.0f;
			blurShader.setVec2("rectMin", glm::vec2(rectMinX, rectMinY));
			blurShader.setVec2("rectMax", glm::vec2(rectMaxX, rectMaxY));

			glDisable(GL_DEPTH_TEST);

			// --- Background (invariato) ---
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


			// --- Matrici per ingredienti (invariato) ---
			glm::mat4 ingProj = glm::ortho(0.0f, (float)screen.w, 0.0f, (float)screen.h, -10.0f, 10.0f);
			glm::mat4 ingView(1.0f);
			glm::mat4 perspectiveProj = glm::perspective(glm::radians(camera.Zoom), (float)screen.w / (float)screen.h, 0.1f, 100.0f);
			glm::mat4 view = camera.GetViewMatrix();
			model = glm::scale(model, glm::vec3(20.0f));

			spawnTimer += deltaTime;
			if (spawnTimer >= spawnInterval) {
				SpawnRandomIngredientWithId(ingredients, ingredientIds);
				spawnTimer = 0.0f;
			}


			// --- Disegno/Logica ingredienti ---
			if (!ingredients.empty()) {
				glEnable(GL_DEPTH_TEST);
				ourShader.use();
				ourShader.setVec3("diffuseColor", glm::vec3(1.0f));
				ourShader.setBool("hasTexture", true);

				glm::mat4 perspectiveProj = glm::perspective(glm::radians(camera.Zoom), (float)screen.w / (float)screen.h, 0.1f, 100.0f);
				glm::mat4 view = camera.GetViewMatrix();

				// Input per taglio
				if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
					if (!isDragging) {
						isDragging = true;
						mouseTrail.clear();
					}
				} else {
					if (isDragging) {
						isDragging = false;
						if (!mouseTrail.empty() && camera.Zoom != 0.0f) {
							// PASSA ANCHE ingredientIds
							scoreManager.processSlash(
								mouseTrail, ingProj, ingView, focusBox,
								ingredients, ingredientIds,          // <-- CAMBIATO
								activeCuts, screen, now, gameState
							);
						}
						mouseTrail.clear();
					}
				}

				// Aggiorna e disegna ingredienti
				objBlurShader.use();
				objBlurShader.setVec2("uInvViewport", glm::vec2(1.0f / screen.w, 1.0f / screen.h));
				objBlurShader.setMat4("projection", ingProj);
				objBlurShader.setMat4("view", ingView);
				objBlurShader.setVec2("rectMin", glm::vec2(rectMinX, rectMinY));
				objBlurShader.setVec2("rectMax", glm::vec2(rectMaxX, rectMaxY));

				glDisable(GL_DEPTH_TEST);
				for (auto& ing : ingredients) {
					ing.Move();
					Texture* t = ing.getFirstTexture();
					if (t != nullptr) {
						objBlurShader.setVec2("uTexelSize", glm::vec2(1.0f / t->w, 1.0f / t->h));
					}
					objBlurShader.setMat4("model", ing.GetModelMatrix());
					objBlurShader.setBool("hasTexture", true);
					objBlurShader.setVec3("diffuseColor", glm::vec3(1.0f));
					ing.Draw(objBlurShader);
				}
			}

			// --- UI: vite + punteggio (invariato) ---
			glDisable(GL_DEPTH_TEST);
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

			textShader.use();
			textShader.setMat4("projection", glm::ortho(0.0f, (float)screen.w, 0.0f, (float)screen.h));
			std::string scoreText = "Score: " + std::to_string(scoreManager.getScore());
			textRenderer.DrawText(textShader, scoreText, screen.w - 250.0f, screen.h - 60.0f, 1.0f, glm::vec3(1.0f, 1.0f, 0.4f));
			
			// --- Lista ingredienti richiesti con quantità rimanente, sotto "Score" ---
			{
				const float startX = screen.w - 250.0f;   // allinea con lo "Score"
				float y = screen.h - 100.0f;              // un po' sotto la riga dello score
				const float lineStep = 28.0f;             // distanza tra righe

				for (const auto& id : recipeIds) {
					int req = scoreManager.getRequiredQty(id);
					int got = scoreManager.getCollectedQty(id);
					int rem = std::max(0, req - got);

					// colore: bianco se ancora da prendere, verdino se completato
					glm::vec3 col = (rem > 0) ? glm::vec3(1.0f) : glm::vec3(0.6f, 1.0f, 0.6f);

					// es. "latte x1"
					std::string line = id + " x" + std::to_string(rem);
					textRenderer.DrawText(textShader, line, startX, y, 0.9f, col);
					y -= lineStep;
				}
			}

			// --- Disegno scia mouse + processSlash in drag (passa ingredientIds) ---
			if (isDragging && mouseTrail.size() >= 2) {
				// 1) scia
				trailShader->use();
				glm::mat4 proj = glm::ortho(0.0f, (float)screen.w, 0.0f, (float)screen.h);
				trailShader->setMat4("projection", proj);
				trailShader->setVec3("trailColor", glm::vec3(1.0f, 1.0f, 0.4f));
				std::vector<float> trailData;
				for (const auto& pt : mouseTrail) { trailData.push_back(pt.x); trailData.push_back(pt.y); }
				glBindBuffer(GL_ARRAY_BUFFER, trailVBO);
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * trailData.size(), trailData.data());
				glBindVertexArray(trailVAO);
				glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)mouseTrail.size());
				glBindVertexArray(0);

				// 2) processa il taglio
				scoreManager.processSlash(
					mouseTrail, ingProj, ingView, focusBox,
					ingredients, ingredientIds,              // <-- CAMBIATO
					activeCuts, screen, now, gameState
				);
			}

			// --- Effetti taglio (invariato) ---
			glLineWidth(4.0f);
			trailShader->use();
			trailShader->setVec3("trailColor", glm::vec3(1.0f, 1.0f, 0.4f));
			trailShader->setMat4("projection", glm::ortho(0.0f, (float)screen.w, 0.0f, (float)screen.h));

			std::vector<float> cutData;
			for (auto it = activeCuts.begin(); it != activeCuts.end(); ) {
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

		else if (gameState == GameState::CONGRATULATIONS) {
			const double now = glfwGetTime();

			// background (riusa quello di PLAYING)
			blurShader.use();
			blurShader.setVec2("uTexelSize", glm::vec2(1.0f / screen.w, 1.0f / screen.h));
			
			blurShader.setVec2("rectMin", glm::vec2(-1.0f, -1.0f));
            blurShader.setVec2("rectMax", glm::vec2(-1.0f, -1.0f));
			
			glm::mat4 orthoProj = glm::ortho(0.0f, (float)screen.w, 0.0f, (float)screen.h, -10.0f, 10.0f);
			blurShader.setMat4("projection", orthoProj);
			blurShader.setMat4("view", glm::mat4(1.0f));

			glm::mat4 model(1.0f);
			model = glm::translate(model, glm::vec3(screen.w / 2.0f, screen.h / 2.0f, 0.0f));
			model = glm::scale(model, glm::vec3(screen.w / 3.2f, screen.h / 1.8f, 1.0f));
			blurShader.setMat4("model", model);
			blurShader.setBool("hasTexture", true);
			blurShader.setVec3("diffuseColor", glm::vec3(1.0f));
			backgroundPlane.Draw(blurShader);

			// Mostra il modello del livello corrente (niente testo)
			glDisable(GL_DEPTH_TEST);
			ourShader.use();
			ourShader.setMat4("projection", orthoProj);
			ourShader.setMat4("view", glm::mat4(1.0f));
			ourShader.setBool("hasTexture", true);
			ourShader.setVec3("diffuseColor", glm::vec3(1.0f));

			glm::mat4 lm(1.0f);
			lm = glm::translate(lm, glm::vec3(screen.w * 0.5f, screen.h * 0.5f, 0.0f));
			float s = std::min(screen.w, screen.h) * 0.20f; // scala “comoda”, regola se serve
			lm = glm::scale(lm, glm::vec3(s, s, 1.0f));
			ourShader.setMat4("model", lm);

			// Disegna "resources/levels/livelloX.obj"
			if (currentLevel >= 1 && currentLevel <= (int)levelModels.size()) {
				levelModels[currentLevel - 1]->Draw(ourShader);
			}

			// entra in congrats al primo frame
			if (congratsStartTime == 0.0) congratsStartTime = now;

			// dopo 5s, passa al livello successivo o a NAME_INPUT
			if (now - congratsStartTime >= kCongratsTime) {
				congratsStartTime = 0.0;
				if (currentLevel >= 10) {
					gameState = GameState::NAME_INPUT; // ultimo livello completato
				} else {
					++currentLevel;
					applyLevel(currentLevel);

					ingredients.clear();
					ingredientIds.clear();
					activeCuts.clear();
					spawnTimer = 0.0f;

					recipeStartTime = glfwGetTime();
					gameState = GameState::RECIPE;
				}
			}
		}


        else if (gameState == GameState::SCORES) {
			
			ourShader.use();
			////////////////////////
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
			ourShader.setVec3("diffuseColor", glm::vec3(2.0f)); // fallback nel caso
			backgroundPlane.Draw(ourShader);
			
			// 1) Posizionamento pergamena
	
            // 0) stato GL sicuro
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);                 // evita scomparse per winding
            
            ourShader.use();
            orthoProj = glm::ortho(0.0f, (float)screen.w, 0.0f, (float)screen.h, -10.0f, 10.0f);
            ourShader.setMat4("projection", orthoProj);
            ourShader.setMat4("view", glm::mat4(1.0f));

			float pW = screen.w * 0.62f;
			float pH = screen.h * 0.72f;
			float pX = (screen.w - pW) * 0.5f;
			float pY = (screen.h - pH) * 0.5f;
            float kH = 0.27f;  
            float kW = 0.3f;
            glm::mat4 pm(1.0f);
            pm = glm::translate(pm, glm::vec3(pX + pW * 0.5f, pY + pH * 0.5f, 0.0f));
            pm = glm::scale(pm, glm::vec3(pW * kW, pH * kH, 1.0f));  // SCALA DELL’OBJ
            ourShader.setMat4("model", pm);

            // vogliamo vedere la TEXTURE della pergamena (se l’OBJ/MTL la fornisce)
            ourShader.setBool("hasTexture", true);
            ourShader.setVec3("diffuseColor", glm::vec3(1.0f));  // nessun tint
            parchment.Draw(ourShader);
			
            // ---------------------------
            // 2) Area interna di scrittura
            // ---------------------------
             // Padding per stare dentro alla pergamena (regola a gusto)
            float padL = 160.0f, padR = 90.0f, padT = 90.0f, padB = 60.0f;
            float innerX = pX + padL;
            float innerY = pY + padB;
            float innerW = pW - (padL + padR);
            float innerH = pH - (padT + padB);


            // Header "Classifica" fisso dentro la pergamena
            textShader.use();
            textShader.setMat4("projection", orthoProj);
            textRenderer.DrawText(textShader, "HIGH SCORE", innerX, innerY + innerH + 40.0f, 1.2f, glm::vec3(0.25f, 0.15f, 0.06f));

            // Zona scrollabile: parte sotto il titolo
            float headerH = 56.0f;               // altezza riservata al titolo dentro la pergamena
            float listTop = innerY + innerH - headerH; // bordo superiore area lista

            // ----- Bordo arrotondato intorno all'area lista -----
            std::vector<glm::vec2> borderPts;

            // leggero inset così non “tocca” il testo
            float inset = 1.0f;
            float bx = innerX + inset;
            float by = innerY + inset;
            float bw = innerW - 70.0f * inset;
            float bh = (listTop - by) - 2.0f * inset;
            
            float listBottom = by;                 // bordo inferiore area lista

            float radius = 22.0f;   // raggio angoli (tuning)
            int   segments = 10;      // più alto = più liscio

            buildRoundedRectOutline(bx, by, bw, bh, radius, segments, borderPts);

            glEnable(GL_LINE_SMOOTH);
            glLineWidth(3.0f);

            // trailShader: lo stai già creando sopra; lo riutilizziamo come line-shader 2D
            trailShader->use();
            // proiezione ortho in pixel
            trailShader->setMat4("projection", orthoProj);
            // se il tuo mouseTrail.fs ha un uniform "color", imposta qui:
            trailShader->setVec3("color", glm::vec3(0.22f, 0.12f, 0.05f)); // marrone scuro

            glBindVertexArray(trailVAO);
            glBindBuffer(GL_ARRAY_BUFFER, trailVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, borderPts.size() * sizeof(glm::vec2), borderPts.data());
            glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)borderPts.size());
            glBindVertexArray(0);

            // ---------------------------------------
            // 3) Scissor per CLIPPING dentro pergamena
            //    (attenzione ai padding del viewport!)
            // ---------------------------------------
            glEnable(GL_SCISSOR_TEST);
            int scX = (int)(innerX + screen.paddingW);
            int scY = (int)(innerY + screen.paddingH);
            int scW = (int)innerW;
            int scH = (int)(listTop - listBottom);
            glScissor(scX, scY, scW, scH);

            // ---------------------------------------
            // 4) Disegno della lista scrollabile
            // ---------------------------------------
            ScoreManager manager;
            auto scores = manager.LoadScores("score.txt");

            float rowH = 62.0f;
            float xRank = innerX + 8.0f;
            float xAvatar = xRank;
            float avatarSize = 48.0f;
            float xName = xAvatar + avatarSize + 12.0f;
            float pillW = 110.0f;
            float xScore = innerX + innerW - pillW - 50.0f;

            // baseline della PRIMA riga (subito sotto listTop).
            // NOTA direzione: con la formula qui sotto, se scoreScrollOffset aumenta, le righe SCENDONO.
            float y = listTop - rowH + scoreScrollOffset;

            int index = 1;
            for (const auto& entry : scores) {
                // (Ottimizzazione) salta righe completamente fuori dal clip verticale
                if (y + rowH <= listBottom || y >= listTop) {
                    y -= rowH;
                    index++;
                    continue;  // fuori dall'area: non disegniamo (ottimizzazione)
                }
                // Riga alternata chiara
                ourShader.use();
                ourShader.setMat4("projection", orthoProj);
                ourShader.setMat4("view", glm::mat4(1.0f));
                ourShader.setBool("hasTexture", false);
                ourShader.setVec3("diffuseColor", (index % 2 == 0) ? glm::vec3(1.0f, 1.0f, 1.0f)
                    : glm::vec3(0.98f, 0.96f, 0.92f));
                
                // Separatore 
                float sepMargin = 180.0f;                  // <-- regola quanto "accorciare"
                float sepW = bw - sepMargin;

                glm::mat4 sm(1.0f);
                sm = glm::translate(sm, glm::vec3(innerX + sepW, y + 2.0f, 0.0f));
                sm = glm::scale(sm, glm::vec3(sepW, 2.0f, 1.0f));
                ourShader.setMat4("model", sm);
                ourShader.setVec3("diffuseColor", glm::vec3(1.0f));
                //backgroundPlane.Draw(ourShader);
               
                textRenderer.DrawText(textShader, std::to_string(index), xRank + 30 , y + 18.0f, 0.9f, glm::vec3(0.12f, 0.08f, 0.06f));

             
                // Nome
                textRenderer.DrawText(textShader, entry.name, xName+70, y + 20.0f, 0.95f, glm::vec3(0.12f, 0.08f, 0.06f));

                //Score
                textRenderer.DrawText(textShader, std::to_string(entry.score), xScore  , y + 20.0f, 1.0f, glm::vec3(1.0f));

                y -= rowH; // riga successiva
                index++;
            }
            glDisable(GL_SCISSOR_TEST);

           
            if (textRenderer.Characters.empty()) {
                std::cerr << "Font non inizializzato!\n";
                continue;
            }

		}
		else if (gameState == GameState::NAME_INPUT) {
			glDisable(GL_DEPTH_TEST);
			textShader.use();
            ourShader.use();
            ////////////////////////

            glm::mat4 orthoProj = glm::ortho(0.0f, (float)screen.w, 0.0f, (float)screen.h, -10.0f, 10.0f);
            ourShader.setMat4("projection", orthoProj);
            ourShader.setMat4("view", glm::mat4(1.0f));

            // Background
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(screen.w / 2.0f, screen.h / 2.0f, 0.0f));
			model = glm::scale(model, glm::vec3(screen.w / 3.2f, screen.h / 1.8f, 1.0f));
			ourShader.setMat4("model", model);
			ourShader.setBool("hasTexture", true);  // background ha texture
			ourShader.setVec3("diffuseColor", glm::vec3(2.0f)); // fallback nel caso
			backgroundPlane.Draw(ourShader);
			// Titolo
			textRenderer.DrawText(textShader, "INSERISCI NOME:", screen.w / 3, screen.h / 2 + 50, 1.0f, glm::vec3(1.0f));

			// Mostra testo digitato
			textRenderer.DrawText(textShader, inputName + "_", screen.w / 3, screen.h / 2, 1.0f, glm::vec3(0.5f, 1.0f, 0.5f));

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
    if (gameState == GameState::SCORES) {
        // Rotellina GIU' (yoffset < 0) -> contenuto scende -> offset AUMENTA
        scoreScrollOffset -= (float)yoffset * 30.0f;

        // Clamp dinamico: totale righe - area visibile pergamena
        ScoreManager manager;
        auto scores = manager.LoadScores("score.txt");

        float rowH = 62.0f;
        float total = (float)scores.size() * rowH;

        // Deve combaciare con l'innerH - headerH usati nel render
        float pH = screen.h * 0.72f;
        float padT = 90.0f, padB = 60.0f, headerH = 56.0f;
        float visible = (pH - (padT + padB)) - headerH;

        float maxScroll = glm::max(0.0f, total - visible);
        scoreScrollOffset = glm::clamp(scoreScrollOffset, 0.0f, maxScroll);
    }
}



