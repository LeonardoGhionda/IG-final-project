#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <random>
#include <vector>
#include <unordered_map>
#include <memory>

//sound
#include <irrKlang.h>

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
#include "backbutton.h"

void OnFramebufferSize(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window, FocusBox& focusBox);
void character_callback(GLFWwindow* window, unsigned int codepoint);
bool customWindowShouldClose(GLFWwindow* window);
void SpawnRandomIngredientWithId(std::vector<Ingredient>& ingredients,
                                 std::vector<std::string>& ingredientIds, int currentLevel);

Screen screen = Screen::S16_9();

float lastX = screen.w / 2.0f;
float lastY = screen.h / 2.0f;
bool firstMouse = true;

Camera camera(CAMERA_POS);

float focusSpeed = 0.8f;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// spawn
float spawnInterval = 1.2f;
float spawnTimer = 0.0f;

TextRenderer textRenderer;

bool endGame = false;
std::string inputName = "";

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


GameState gameState = GameState::START;
std::vector<Ingredient> ingredients;

double startScreenStartTime = 0.0;

static float frand(float a, float b) {
	static std::mt19937 gen{ std::random_device{}() };
	std::uniform_real_distribution<float> d(a, b);
	return d(gen);
}

//sound
irrklang::ISoundEngine* engine;

//Funzioni
bool customWindowShouldClose(GLFWwindow* window) {
    return glfwWindowShouldClose(window);
}

void SpawnRandomIngredientWithId(std::vector<Ingredient>& ingredients,
                                 std::vector<std::string>& ingredientIds, int currentLevel)
{
    struct SpawnDef {
        const char* path;
        const char* id;
        float scale = 30.0f;
        bool isBomb = false;
    };

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

    static const SpawnDef kBomb  = { "resources/ingredients/bomb/bomb.obj",  "bomb",  30.0f, true  };
    static const SpawnDef kHeart = { "resources/ingredients/heart/heart.obj","heart", 28.0f, false };

    // ------------------ difficoltà in base al livello ------------------
 
    const int lvl = std::max(1, currentLevel);

    // Probabilità (percento)
    const int heartPct = std::max(2, 5 - (lvl / 3));                 // 6% → 5% → ... → min 2%
    const int bombPct  = std::min(38, 8 + 3 * (lvl - 1));           // 12% +2%/lvl → max 38%

    // Scelta categoriale: prima cuore, poi bomba, altrimenti ingrediente
    std::uniform_int_distribution<int> d100(0, 99);
    int roll = d100(gen);

    SpawnDef chosen;
    if (roll < heartPct) {
        chosen = kHeart;
    } else if (roll < heartPct + bombPct) {
        chosen = kBomb;
    }
	else {
		
		std::vector<double> weights;
		weights.reserve(kNonBomb.size());

		for (const auto& def : kNonBomb) {
			int qty = scoreManager.getRequiredQty(def.id);
			//ingredienti ricetta pesano di più (1 + 2 * qty)
			double w = (qty > 0) ? (1.0 + qty * 2.0) : 1.0;
			weights.push_back(w);
		}

		std::discrete_distribution<size_t> pick(weights.begin(), weights.end());
		chosen = kNonBomb[pick(gen)];
	}

    // ------------------ spawn point e direzione ------------------
    glm::vec2 spawn = Ingredient::RandomSpawnPoint();

	glm::vec2 target = { frand(0.0f, screen.w), frand(0.0f, screen.h) };

    glm::vec2 dir = glm::normalize(target - spawn);

    std::uniform_real_distribution<float> angleDeg(-12.0f, 12.0f);
    dir = RotateVec2(dir, glm::radians(angleDeg(gen)));

    // ------------------ velocità con scala sul livello ------------------
    std::uniform_real_distribution<float> speedJit(600.0f, 700.0f);
    float speed = speedJit(gen) * (0.7f + 0.07f * (lvl - 1));        // +7% per livello
    speed = std::min(speed, 1400.0f);                                // clamp di sicurezza

    Ingredient item(chosen.path, spawn, chosen.scale, chosen.isBomb);
    item.SetVelocity(dir * speed);
    item.updateTime();

    ingredients.emplace_back(std::move(item));
    ingredientIds.emplace_back(chosen.id);
}


void character_callback(GLFWwindow* window, unsigned int codepoint) {
    if (gameState != GameState::NAME_INPUT) return;

    // Accetta solo caratteri stampabili ASCII
    if (codepoint >= 32 && codepoint <= 126) {
        if (inputName.size() < 10) {
            inputName += static_cast<char>(codepoint);
        }
    }
}

// contorno rettangolare con angoli smussati
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

int main() {
    // glfw: init
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
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

	//sound
	engine = irrklang::createIrrKlangDevice();
	if (!engine)
		return 0; // error starting up the engine

	engine->play2D("resources/sounds/bgmusic.ogg", true);

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
	Shader blurShader("shader.vs", "shader_blur.fs");
	Shader objBlurShader("shader.vs", "shader_blur_objs.fs");
	Shader lambertShader("lambert.vs", "lambert.fs");
	Shader uiShader("ui.vs", "ui.fs");
	// load models
	// -----------
	Model backgroundPlane("resources/background/table.obj");
	Model playButtonModel("resources/buttons/play.obj");
	Model scoresButtonModel("resources/buttons/scores.obj");
	Model infoButtonModel("resources/buttons/info.obj");
    Model lifeIcon("resources/ingredients/heart/heart.obj");
	Model parchment("resources/recipes/pergamena.obj");
	Model pauseModel("resources/levels/pause.obj");
	Model rulesModel("resources/recipes/rules.obj");
	Model startBackground("resources/background/startBackground.obj");
	Model apple3d("resources/3d/apple3d.obj");

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
	constexpr double kCongratsTime  = 10.0;

	// timestamp inizio stato RECIPE
	int currentLevel = 1;
	double recipeStartTime = 0.0;
	double congratsStartTime = 0.0;


	// vettore parallelo agli ingredienti per tenerne l'ID
	std::vector<std::string> ingredientIds;                 
	std::vector<std::string> recipeIds;

	std::vector<std::unordered_map<std::string,int>> levelRecipes = {
		{{"flour",2}, {"milk",3}, {"eggs",5}, {"lemon",1}, {"butter",2}, {"vanilla",1}},                                  // 1
		{{"flour",2}, {"milk",2}, {"eggs",4}, {"apple",5}, {"butter",3}},                     // 2
		{{"flour",2}, {"milk",1}, {"eggs",3}, {"strawberry",5}, {"jam",4}},                                            // 3
		{{"pumpkin",2}, {"milk",3}, {"vanilla",1}, {"honey",4}},                           // 4
		{{"milk",3}, {"lemon",5}, {"honey",4}},                  // 5
		{{"chocolate",4}, {"eggs",2}, {"butter",4}, {"flour",3}},                             // 6
		{{"strawberry",4}, {"lemon",3}, {"apple",2}, {"honey",2}},                               // 7
		{{"chocolate",2}, {"milk",3}, {"flour",3}, {"eggs",2}},                                             // 8
		{{"milk",3}, {"chocolate",3}, {"strawberry",4}},                                   // 9
		{{"chocolate",3}, {"vanilla",3}, {"pumpkin",4}, {"eggs",3}}       // 10
	};

	auto applyLevel = [&](int lvl){
		scoreManager.setRequiredRecipe(levelRecipes[lvl-1]);
		recipeIds.clear();
		for (auto &kv : levelRecipes[lvl-1]) recipeIds.push_back(kv.first);
	}; 

   
	glm::vec2 baseSize = glm::vec2(100.0f, 50.0f);
	float playYScale = 2.0f;
	float scoresYScale = 2.0f;
	float infoYScale = 2.0f;

    Button playButton{  glm::vec2(0.5f, 0.75f), baseSize, playYScale,   "PLAY"   };
    Button scoresButton{glm::vec2(0.5f, 0.50f), baseSize, scoresYScale, "SCORES" };
    Button infoButton{  glm::vec2(0.5f, 0.25f), baseSize, infoYScale,   "INFO"   };

	//backbutton
	BackButton backButton("resources/buttons/backbutton.png", glm::vec2(64.0f, 64.0f), glm::vec2(screen.w, screen.h));

	//APPLE INITIALIZATION
	glm::mat4 apple_mat = glm::mat4(1.0f);
	//pos
	glm::vec3 apple_dir = glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f));
	apple_mat = glm::translate(apple_mat, glm::vec3(10.0f, 10.0f, 0.0f));
	//scale
	float base_scale = screen.w / screen.ARW;
	apple_mat = glm::scale(apple_mat, glm::vec3(base_scale * 20.0f));
	// posizione iniziale
	glm::vec3 apple_pos = glm::vec3(10.0f, 10.0f, 0.0f);
	// accumulatore per rotazione
	float apple_angle_accum = 0.0f;
	// asse di rotazione (fisso o random)
	glm::vec3 apple_axis = glm::normalize(glm::vec3(1.0f, 0.8f, 0.6f));

    // MAIN LOOP
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

            glm::mat4 orthoProj = glm::ortho(0.0f, (float)screen.w, 0.0f, (float)screen.h, -1000.0f, 1000.0f);
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

			//APPLE
			lambertShader.use();
			//rotazione
			apple_angle_accum += glm::radians(50.0f) * deltaTime;
			//rimbalzi
			if (apple_pos.x > screen.w || apple_pos.x < 0.0f)
				apple_dir.x *= -1.0f;
			if (apple_pos.y > screen.h || apple_pos.y < 0.0f)
				apple_dir.y *= -1.0f;

			//posizione
			float apple_speed = 50.0f; // pixel al secondo
			apple_pos += apple_dir * apple_speed * deltaTime;
			glm::mat4 apple_mat = glm::mat4(1.0f);
			//traslzione
			apple_mat = glm::translate(apple_mat, apple_pos);
			// rotazione attorno al proprio asse
			apple_mat = glm::rotate(apple_mat, apple_angle_accum, apple_axis);
			// scala
			float base_scale = screen.w / screen.ARW;
			apple_mat = glm::scale(apple_mat, glm::vec3(base_scale * 20.0f));

			lambertShader.setMat4("model", apple_mat);
			lambertShader.setMat4("projection", orthoProj);
			lambertShader.setMat4("view", glm::mat4(1.0f));
			lambertShader.setBool("hasTexture", true);
			lambertShader.setVec3("diffuseColor", glm::vec3(1.0f));
			lambertShader.setVec3("lightPos", glm::vec3((float)screen.w / 2, (float)screen.h / 2, 4));

			// se la mela ha texture
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, apple3d.textures_loaded[0].id);
			lambertShader.setInt("diffuseMap", 0);

			// attiva il depth test così la mela si occlude correttamente
			glEnable(GL_DEPTH_TEST);
			apple3d.Draw(lambertShader);
			glDisable(GL_DEPTH_TEST);

			ourShader.use();

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
		else if (gameState == GameState::START) {
			focusBox.resetSize();

			const double now = glfwGetTime();
			glDisable(GL_DEPTH_TEST);

			// proiezione ortografica in pixel
			glm::mat4 orthoProj = glm::ortho(0.0f, (float)screen.w, 0.0f, (float)screen.h, -10.0f, 10.0f);

			// disegna background completamente adattato alla finestra
			ourShader.use();
			ourShader.setMat4("projection", orthoProj);
			ourShader.setMat4("view", glm::mat4(1.0f));
			ourShader.setBool("hasTexture", true);
			ourShader.setVec3("diffuseColor", glm::vec3(1.0f));

			glm::mat4 m(1.0f);
			m = glm::translate(m, glm::vec3(screen.w * 0.5f, screen.h * 0.5f, 0.0f));
			m = glm::scale(m, glm::vec3(screen.w, screen.h, 1.0f));
			ourShader.setMat4("model", m);
			startBackground.Draw(ourShader);

			// --- Timer di ingresso (prima volta) ---
			if (startScreenStartTime == 0.0) {
				startScreenStartTime = now;
			}

			// --- Uscita: click o dopo 5 secondi ---
			if (keys.PressedAndReleased(GLFW_MOUSE_BUTTON_LEFT) ||
				(now - startScreenStartTime) >= 5.0) {
				startScreenStartTime = 0.0;
				gameState = GameState::MENU;
				continue; // evita di processare click sul MENU nello stesso frame
			}
		}

		else if (gameState == GameState::RECIPE) {
			focusBox.resetSize();

			const double now = glfwGetTime();

			// -- background come in PLAYING --
			blurShader.use();
			blurShader.setVec2("uTexelSize", glm::vec2(1.0f / screen.w, 1.0f / screen.h));

			blurShader.setVec2("rectMin", glm::vec2(-1.0f, -1.0f));
    		blurShader.setVec2("rectMax", glm::vec2(-1.0f, -1.0f));
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

			// -- mostra ricetta1.obj 
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
			textRenderer.DrawText(textShader, std::to_string(remaining),
								screen.w - 70.0f, screen.h - 60.0f, 1.0f, glm::vec3(1.0f, 1.0f, 0.4f));

			// dopo 3s vai a PLAYING
			if (now - recipeStartTime >= kRecipePreview) {
				spawnTimer = 0.0f;              
				glEnable(GL_DEPTH_TEST);
				gameState = GameState::PLAYING;
				continue;                       // passa subito al frame PLAYING
			}
		}
        else if (gameState == GameState::PLAYING) {
			const double now = glfwGetTime();

			// --- Blur pass + focus rect---
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

			// --- Background ---
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


			// --- Matrici per ingredienti ---
			glm::mat4 ingProj = glm::ortho(0.0f, (float)screen.w, 0.0f, (float)screen.h, -10.0f, 10.0f);
			glm::mat4 ingView(1.0f);
			model = glm::scale(model, glm::vec3(20.0f));

			spawnTimer += deltaTime;
			if (spawnTimer >= spawnInterval) {
				SpawnRandomIngredientWithId(ingredients, ingredientIds,currentLevel);
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
								ingredients, ingredientIds,          
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
				rectMinX += static_cast<float>(screen.paddingW)/ screen.w;
				rectMaxX += static_cast<float>(screen.paddingW)/ screen.w;
				rectMinY += static_cast<float>(screen.paddingH)/ screen.h;
				rectMaxY += static_cast<float>(screen.paddingH)/ screen.h;
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
		

			// --- UI: vite + punteggio ---
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
			std::string scoreText = "SCORE " + std::to_string(scoreManager.getScore());
			textRenderer.DrawText(textShader, scoreText, screen.w - 250.0f, screen.h - 60.0f, 1.0f, glm::vec3(1.0f, 1.0f, 0.4f));
			
			// --- Lista ingredienti richiesti con quantità rimanente, sotto "Score" ---
			{
				const float startX = screen.w - 180.0f;   // allinea con lo "Score"
				float y = screen.h - 110.0f;              // un po' sotto la riga dello score
				const float lineStep = 30.0f;             

				for (const auto& id : recipeIds) {
					int req = scoreManager.getRequiredQty(id);
					int got = scoreManager.getCollectedQty(id);
					int rem = std::max(0, req - got);

					// colore: bianco se ancora da prendere, verdino se completato
					glm::vec3 col = (rem > 0) ? glm::vec3(1.0f) : glm::vec3(0.6f, 1.0f, 0.6f);

					std::string line = id + " x" + std::to_string(rem);
					textRenderer.DrawText(textShader, line, startX, y, 0.5f, col);
					y -= lineStep;
				}
			}

			// --- Disegno scia mouse + processSlash in drag ---
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
					ingredients, ingredientIds,            
					activeCuts, screen, now, gameState
				);
			}

			// --- Effetti taglio) ---
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

			// background
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

			if (currentLevel >= 1 && currentLevel <= (int)levelModels.size()) {
				levelModels[currentLevel - 1]->Draw(ourShader);
			}

			if (congratsStartTime == 0.0) congratsStartTime = now;

			// dopo 5s
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
			glDisable(GL_DEPTH_TEST);

			glm::mat4 orthoProj = glm::ortho(0.0f, (float)screen.w, 0.0f, (float)screen.h, -10.0f, 10.0f);
			ourShader.setMat4("projection", orthoProj);
			ourShader.setMat4("view", glm::mat4(1.0f));

			// Background
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(screen.w / 2.0f, screen.h / 2.0f, 0.0f));
			model = glm::scale(model, glm::vec3(screen.w / 3.2f, screen.h / 1.8f, 1.0f));
			ourShader.setMat4("model", model);
			ourShader.setBool("hasTexture", true);  
			ourShader.setVec3("diffuseColor", glm::vec3(2.0f)); 
			backgroundPlane.Draw(ourShader);
			
			// 1) Posizionamento pergamena
	
            // 0) stato GL sicuro
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);                 
            
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
            pm = glm::scale(pm, glm::vec3(pW * kW, pH * kH, 1.0f));  
            ourShader.setMat4("model", pm);
            ourShader.setBool("hasTexture", true);
            ourShader.setVec3("diffuseColor", glm::vec3(1.0f));  
            parchment.Draw(ourShader);
			
            // ---------------------------
            // 2) Area interna di scrittura
            // ---------------------------
             // Padding per stare dentro alla pergamena
            float padL = 160.0f, padR = 90.0f, padT = 90.0f, padB = 60.0f;
            float innerX = pX + padL;
            float innerY = pY + padB;
            float innerW = pW - (padL + padR);
            float innerH = pH - (padT + padB);


            // Helper per misurare la larghezza del testo (in px)
			auto textWidthPx = [&](const std::string& s, float scale) {
				float w = 0.0f;
				for (char c : s) {
					auto it = textRenderer.Characters.find(c);
					if (it != textRenderer.Characters.end())
						w += (it->second.Advance >> 6) * scale; // Advance è in 1/64 px
				}
				return w;
			};

			textShader.use();
			textShader.setMat4("projection", orthoProj);

			const float headerScale = 1.2f;
			const std::string header = "HIGH SCORE";

			// X centrata rispetto allo schermo
			float headerX = (screen.w - textWidthPx(header, headerScale)) * 0.5f;
			// Y dove lo avevi (sopra l’area lista)
			float headerY = innerY + innerH + 40.0f;

			textRenderer.DrawText(textShader, header, headerX, headerY, headerScale,
								glm::vec3(0.25f, 0.15f, 0.06f));


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
            float xName =60.0f+xRank;
            float pillW = 110.0f;
            float xScore = innerX + innerW - pillW - 50.0f;

            float y = listTop - rowH + scoreScrollOffset;

            int index = 1;
            for (const auto& entry : scores) {
                if (y + rowH <= listBottom || y >= listTop) {
                    y -= rowH;
                    index++;
                    continue; 
                }
                ourShader.use();
                ourShader.setMat4("projection", orthoProj);
                ourShader.setMat4("view", glm::mat4(1.0f));
                ourShader.setBool("hasTexture", false);
                ourShader.setVec3("diffuseColor", (index % 2 == 0) ? glm::vec3(1.0f, 1.0f, 1.0f)
                    : glm::vec3(0.98f, 0.96f, 0.92f));
                
                     
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

			//backbutton
			glDisable(GL_DEPTH_TEST);
			backButton.ProcessInput(window, glm::vec2(screen.w, screen.h), GameState::MENU);
			backButton.Draw(uiShader, glm::vec2(screen.w, screen.h));
			glEnable(GL_DEPTH_TEST);

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

			// Titolo + input centrati orizzontalmente
			textShader.use();
			textShader.setMat4("projection", orthoProj);

			// Helper per misurare la larghezza del testo in pixel
			auto textWidthPx = [&](const std::string& s, float scale) -> float {
				float w = 0.0f;
				for (char c : s) {
					auto it = textRenderer.Characters.find(c);
					if (it != textRenderer.Characters.end()) {
						w += (it->second.Advance >> 6) * scale; // Advance è in 1/64 px
					}
				}
				return w;
			};

			const float titleScale = 1.0f;
			const float inputScale = 1.0f;

			std::string title = "INSERISCI NOME:";
			std::string input = inputName + "_";

			// X centrata: (larghezza schermo - larghezza testo)/2
			float titleX = (screen.w - textWidthPx(title, titleScale)) * 0.5f;
			float inputX = (screen.w - textWidthPx(input, inputScale)) * 0.5f;

			// Y: due righe attorno al centro dello schermo (leggermente separate)
			float centerY = screen.h * 0.5f;
			float titleY  = centerY + 40.0f;  // sopra al centro
			float inputY  = centerY - 10.0f;  // sotto al centro

			textRenderer.DrawText(textShader, title, titleX, titleY, titleScale, glm::vec3(1.0f));
			textRenderer.DrawText(textShader, input, inputX, inputY, inputScale, glm::vec3(0.5f, 1.0f, 0.5f));


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

			// Background come negli altri menu
			ourShader.use();
			glm::mat4 orthoProj = glm::ortho(0.0f, (float)screen.w, 0.0f, (float)screen.h, -10.0f, 10.0f);
			ourShader.setMat4("projection", orthoProj);
			ourShader.setMat4("view", glm::mat4(1.0f));
			ourShader.setBool("hasTexture", true);
			ourShader.setVec3("diffuseColor", glm::vec3(1.0f));

			glm::mat4 bg(1.0f);
			bg = glm::translate(bg, glm::vec3(screen.w / 2.0f, screen.h / 2.0f, 0.0f));
			bg = glm::scale(bg, glm::vec3(screen.w / 3.2f, screen.h / 1.8f, 1.0f));
			ourShader.setMat4("model", bg);
			backgroundPlane.Draw(ourShader);

			// Modello di pausa centrato
			glm::mat4 pm(1.0f);
			pm = glm::translate(pm, glm::vec3(screen.w * 0.5f, screen.h * 0.5f, 0.0f));
			float s = std::min(screen.w, screen.h) * 0.20f; // regola se serve più grande/piccolo
			pm = glm::scale(pm, glm::vec3(s, s, 1.0f));
			ourShader.setMat4("model", pm);
			pauseModel.Draw(ourShader);
		}
		else if (gameState == GameState::INFO) {
			// Background uguale a PLAYING ma senza focus box
			blurShader.use();
			blurShader.setVec2("uTexelSize", glm::vec2(1.0f / screen.w, 1.0f / screen.h));

			glm::mat4 orthoProj = glm::ortho(0.0f, (float)screen.w, 0.0f, (float)screen.h, -10.0f, 10.0f);
			blurShader.setMat4("projection", orthoProj);
			blurShader.setMat4("view", glm::mat4(1.0f));

			glm::mat4 bg(1.0f);
			bg = glm::translate(bg, glm::vec3(screen.w / 2.0f, screen.h / 2.0f, 0.0f));
			bg = glm::scale(bg, glm::vec3(screen.w / 3.2f, screen.h / 1.8f, 1.0f));
			blurShader.setMat4("model", bg);
			blurShader.setBool("hasTexture", true);
			blurShader.setVec3("diffuseColor", glm::vec3(1.0f));
			backgroundPlane.Draw(blurShader);

			// Modello delle regole centrato
			glDisable(GL_DEPTH_TEST);
			ourShader.use();
			ourShader.setMat4("projection", orthoProj);
			ourShader.setMat4("view", glm::mat4(1.0f));
			ourShader.setBool("hasTexture", true);
			ourShader.setVec3("diffuseColor", glm::vec3(1.0f));

			glm::mat4 m(1.0f);
			m = glm::translate(m, glm::vec3(screen.w * 0.5f, screen.h * 0.5f, 0.0f));
			float s = std::min(screen.w, screen.h) * 0.15f;   // regola se serve
			m = glm::scale(m, glm::vec3(s, s, 1.0f));
			ourShader.setMat4("model", m);

			rulesModel.Draw(ourShader);
			glEnable(GL_DEPTH_TEST);

			glDisable(GL_DEPTH_TEST);
			backButton.ProcessInput(window, glm::vec2(screen.w, screen.h), GameState::MENU);
			backButton.Draw(uiShader, glm::vec2(screen.w, screen.h));
			glEnable(GL_DEPTH_TEST);
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
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) currentSpeed *= 5.0f;

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
        // Rotellina
        scoreScrollOffset -= (float)yoffset * 30.0f;

        // Clamp dinamico: totale righe - area visibile pergamena
        ScoreManager manager;
        auto scores = manager.LoadScores("score.txt");

        float rowH = 62.0f;
        float total = (float)scores.size() * rowH;

        float pH = screen.h * 0.72f;
        float padT = 90.0f, padB = 60.0f, headerH = 56.0f;
        float visible = (pH - (padT + padB)) - headerH;

        float maxScroll = glm::max(0.0f, total - visible);
        scoreScrollOffset = glm::clamp(scoreScrollOffset, 0.0f, maxScroll);
    }
}



