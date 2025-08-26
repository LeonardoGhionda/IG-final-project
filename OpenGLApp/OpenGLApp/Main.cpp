#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "shader.h"
#include "camera.h"
#include "model.h"
#include "ingredient.h"
#include "vecplus.h"
#include "screen.h"
#include "keys.h"
#include <random>
#include <deque>
#include <limits>
#include "ScoreManager.h"
#include "TextRenderer.h"
#include "focusbox.h"
#include "button.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window,FocusBox& focusBox);


float lastX = screen.w / 2.0f;
float lastY = screen.h / 2.0f;
bool firstMouse = true;
Screen screen = Screen::S16_9();

// camera
Camera camera(CAMERA_POS);

float focusSpeed = 400.0f; // puoi metterlo come variabile globale

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;
//spawn
float spawnInterval = 2.0f;   // intervallo tra un lancio e l'altro (in secondi)
float spawnTimer = 0.0f;      // tempo trascorso dall'ultimo lancio


TextRenderer textRenderer;

//score
bool endGame = false;
int score = 0;
std::string inputName = "";
bool isTypingName = false;

//fullscreen
bool fullscreen = false;
bool Esc = false;

glm::vec2 mousePos = glm::vec2(0.0f);
glm::vec2 mouseScreenPos = glm::vec2(0.0f);

Keys keys;

std::random_device rd;
std::mt19937 gen(rd());

enum class GameState {
	MENU,
	PLAYING,
	SCORES,
	INFO,
	PAUSED,
	NAME_INPUT,
};

//mouse trail
std::vector<glm::vec2> mouseTrail;
bool isDragging = false;


GameState gameState = GameState::PLAYING;
std::deque<Ingredient> ingredients;
std::string playerName = "Player1"; // Default player name, can be changed later
//Ingredient* active = nullptr;


bool customWindowShouldClose(GLFWwindow* window) {
	return glfwWindowShouldClose(window);
}

void SpawnRandomIngredient() {
    std::vector<std::string> allIngredients = {
           "resources/ball/ball.obj",
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
         "resources/ingredients/strawberry/strawberry.obj"
        // altri ingredienti qui
    };
   
    int index = rand() % allIngredients.size();
    glm::vec2 spawn = Ingredient::RandomSpawnPoint();

    Ingredient newIngredient(allIngredients[index].c_str(), spawn, 0.3f);


	// Direzione verso l'alto e centro, con piccola deviazione casuale
	glm::vec2 target(0.0f, screen.screenlimit.y * 0.5f);
	glm::vec2 dir = glm::normalize(target - spawn);

	// Aggiungi una leggera deviazione casuale sull'asse X
	float angleOffset = ((rand() % 40) - 20) * 0.01745f; // ±20°
	dir = RotateVec2(dir, angleOffset);

	// Velocità in stile Fruit Ninja (8-12)
	float speed = 8.0f + static_cast<float>(rand() % 40) / 4.0f;

    newIngredient.SetVelocity(dir * speed);
    newIngredient.updateTime();
    ingredients.push_back(newIngredient);
}

void character_callback(GLFWwindow* window, unsigned int codepoint) {
	if (gameState == GameState::NAME_INPUT) {
		if (codepoint == GLFW_KEY_BACKSPACE && !inputName.empty()) {
			inputName.pop_back();
		}
		else if (inputName.size() < 12 && codepoint >= 32 && codepoint <= 126) {
			inputName += static_cast<char>(codepoint);
		}
	}
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

	std::ifstream shaderCheck("text.vs");
	if (!shaderCheck.is_open()) {
		std::cerr << " text.vs non trovato nella working directory\n";
	}


	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	// tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	glfwSetCharCallback(window, character_callback);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	FocusBox focusBox(glm::vec2(200.0f, 120.0f)); // larghezza 200, altezza 120
	focusBox.SetCenter(glm::vec2(1.0f / 2.0f, 1.0f / 2.0f)); //cambiato da pixel a percentuale schermo

	Shader textShader("text.vs", "text.fs"); // Shader per testo
	if (!textRenderer.Load("resources/arial.ttf", 48)) {
		std::cerr << "Errore caricamento font!\n";
		return -1;
	}
	textShader.use();
	glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(screen.w), 0.0f, static_cast<float>(screen.h));
	textShader.setMat4("projection", projection);


	// tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
	//stbi_set_flip_vertically_on_load(true);

	// configure global opengl state
	// -----------------------------
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
		glm::vec2(1.0f / 2, 3.0f / 4),
		baseSize,
		playYScale,
		"PLAY"
	};

	Button scoresButton{
		glm::vec2(1.0f / 2, 2.0f / 4),
		baseSize,
		scoresYScale,
		"SCORES"
	};

	Button infoButton{
		glm::vec2(1.0f / 2, 1.0f / 4),
		baseSize,
		infoYScale,
		"INFO"
	};


	// Per bitmap: g->bitmap.buffer
	// Per contorni: g->outline

	// 4. Da outline → mesh
	// g->outline: contiene i contorni vettoriali della lettera
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

        processInput(window,focusBox);

		glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		ourShader.use();

	
		if (gameState == GameState::MENU) {

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
				score = 0;
				spawnTimer = 0.0f; // Reset timer
                gameState = GameState::PLAYING;
                glEnable(GL_DEPTH_TEST);
                continue;
				/*
				//inserisco 4 ingredienti
				for (int i = 0; i < 4; ++i)
					ingredients.push_back(Ingredient("resources/ball/ball.obj", Ingredient::RandomSpawnPoint()));
				active = &ingredients[0];
				active->AddVelocity(active->getDirectionToCenter() * 5.0f);
				active->updateTime();
			   */

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

			blurShader.use();

			blurShader.setVec2("uTexelSize", glm::vec2(1.0f / screen.w, 1.0f / screen.h));

			// rettangolo non soggetto al blur 
			//--------------------------------
			glm::vec2 rectSize = focusBox.getScaledSize() * 2.0f;
			glm::vec2 rectPos = focusBox.GetCenter();
			//normalizzazione della size  
			rectSize.x /= screen.w;
			rectSize.y /= screen.h;
			//invert y 
			rectPos.y = 1 - rectPos.y;
			//calcolo min
			float rectMinX = rectPos.x - rectSize.x / 2.0f;
			if (rectMinX < 0.0f) rectMinX = 0.0f;
			float rectMinY = rectPos.y - rectSize.y / 2.0f;
			if (rectMinY < 0.0f) rectMinY = 0.0f;
			blurShader.setVec2("rectMin", glm::vec2(rectMinX, rectMinY));
			//calcolo max
			float rectMaxX = rectPos.x + rectSize.x / 2.0f;
			if (rectMaxX > 1.0f) rectMaxX = 1.0f;
			float rectMaxY = rectPos.y + rectSize.y / 2.0f;
			if (rectMaxY > 1.0f) rectMaxY = 1.0f;
			blurShader.setVec2("rectMax", glm::vec2(rectMaxX, rectMaxY));
			//------------------------------------------------------------

			glDisable(GL_DEPTH_TEST);

			// Background
			//-----------------
			glm::mat4 orthoProj = glm::ortho(0.0f, (float)screen.w, 0.0f, (float)screen.h, -10.0f, 10.0f);
			blurShader.setMat4("projection", orthoProj);
			blurShader.setMat4("view", glm::mat4(1.0f));


			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(screen.w / 2.0f, screen.h / 2.0f, 0.0f));
			model = glm::scale(model, glm::vec3(screen.w / 3.2f, screen.h / 1.8f, 1.0f));
			blurShader.setMat4("model", model);
			blurShader.setBool("hasTexture", true);  // background ha texture
			blurShader.setVec3("diffuseColor", glm::vec3(1.0f)); // fallback nel caso

			backgroundPlane.Draw(blurShader);


			// Ingredients
			//---------------
			glm::mat4 perspectiveProj = glm::perspective(glm::radians(camera.Zoom), (float)screen.w / (float)screen.h, 0.1f, 100.0f);
			glm::mat4 view = camera.GetViewMatrix();
			spawnTimer += deltaTime;
			if (spawnTimer >= spawnInterval) {
				SpawnRandomIngredient();
				spawnTimer = 0.0f;
			}
			
						/* viene eseguito anche 3 righe sotto, qualcuno sa se serve anche qui? o è quello sopra da togliere?
						* ho tolto questo perchè l'altro ha il controllo su empty
						for (auto& ing : ingredients) {
							ing.Move();
							objBlurShader.setMat4("model", ing.GetModelMatrix());
							objBlurShader.setBool("hasTexture", true);
							objBlurShader.setVec3("diffuseColor", glm::vec3(1.0f));
							ing.Draw(ourShader);
						}
						*/

			if (!ingredients.empty()) {	

				// Aggiorna e disegna tutti gli ingredienti
				objBlurShader.use();

				objBlurShader.setVec2("uInvViewport", glm::vec2(1.0f / screen.w, 1.0f / screen.h));
				
				objBlurShader.setMat4("projection", perspectiveProj);
				objBlurShader.setMat4("view", view);
				objBlurShader.setVec2("rectMin", glm::vec2(rectMinX, rectMinY));
				objBlurShader.setVec2("rectMax", glm::vec2(rectMaxX, rectMaxY));

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

			//Mostra punteggio             
			textShader.use();
			glm::mat4 projection = glm::ortho(0.0f, (float)screen.w, 0.0f, (float)screen.h);
			textShader.setMat4("projection", projection);
			std::string scoreText = "Score: " + std::to_string(score);
			glm::vec3 color = glm::vec3(1.0f); // bianco
			textRenderer.DrawText(textShader, scoreText, screen.w - 200.0f, screen.h - 50.0f, 1.0f, color);

			//move focus box
			// Movimento della focus box
			glm::vec2 moveDelta(0.0f);
			float speed = 0.4f;
			if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) moveDelta.x -= speed * deltaTime;
			if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) moveDelta.x += speed * deltaTime;
			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) moveDelta.y += speed * deltaTime;
			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) moveDelta.y -= speed * deltaTime;

			focusBox.Move(moveDelta);

			focusBox.Draw(focusShader, screen.w, screen.h);

			if (keys.PressedAndReleased(GLFW_MOUSE_BUTTON_LEFT)) {
				for (auto it = ingredients.begin(); it != ingredients.end();) {
					if (it->hit(mouseScreenPos, perspectiveProj, view)) {
						if (focusBox.Contains(mousePos)) {
							score++;
						}
						else {
							score = std::max(0, score - 1);
						}
						it = ingredients.erase(it);
					}

					else {
						++it;
					}
				}
			}


        }
        else if (gameState == GameState::SCORES) {

			ScoreManager manager;
			auto scores = manager.LoadScores("score.txt");

			textShader.use();
			glm::mat4 projection = glm::ortho(0.0f, (float)screen.w, 0.0f, (float)screen.h);
			textShader.setMat4("projection", projection);

			float y = screen.h - 100.0f;
			float x = 100.0f;

			// Titolo classifica
			textRenderer.DrawText(textShader, "PUNTEGGI", x, y, 1.5f, glm::vec3(1.0f, 0.8f, 0.0f));
			y -= 80.0f;

			if (scores.empty()) {
				// Nessun punteggio registrato
				textRenderer.DrawText(textShader, "Nessun punteggio salvato.", x, y, 1.0f, glm::vec3(1.0f));
			}
			else {
				int index = 1;
				for (const auto& entry : scores) {
					std::string line = std::to_string(index++) + ". " + entry.name + ": " + std::to_string(entry.score);
					textRenderer.DrawText(textShader, line, x, y, 1.0f, glm::vec3(1.0f));
					y -= 60.0f;  // Spaziatura tra le righe
				}
			}
			if (textRenderer.Characters.empty()) {
				std::cerr << "Font non inizializzato!\n";
				continue;  // Salta il frame, non blocca il gioco
			}

		}
		else if (gameState == GameState::NAME_INPUT) {
			glDisable(GL_DEPTH_TEST);
			textShader.use();

			// Titolo
			textRenderer.DrawText(textShader, "INSERISCI NOME:", screen.w / 3, screen.h / 2 + 50, 1.0f, glm::vec3(1.0f));

			// Mostra testo digitato
			textRenderer.DrawText(textShader, inputName + "_", screen.w / 3, screen.h / 2, 1.0f, glm::vec3(0.5f, 1.0f, 0.5f));

			// -------------------------------
			// Gestione BACKSPACE
			// -------------------------------
			if (glfwGetKey(window, GLFW_KEY_BACKSPACE) == GLFW_PRESS && !inputName.empty()) {
				static double lastDeleteTime = 0;
				double currentTime = glfwGetTime();

				// Ritardo per evitare cancellazioni multiple in un singolo frame
				if (currentTime - lastDeleteTime > 0.15) {
					inputName.pop_back();
					lastDeleteTime = currentTime;
				}
			}

			// -------------------------------
			// Gestione ENTER (normale + tastierino)
			// -------------------------------
			if ((glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS ||
				glfwGetKey(window, GLFW_KEY_KP_ENTER) == GLFW_PRESS)) {

				// Se nome vuoto, assegna "Anonimo"
				if (inputName.empty()) {
					playerName = "Anonimo";
				}
				else {
					playerName = inputName;
				}

				// Salvataggio punteggio
				ScoreManager manager;
				manager.SaveScore(playerName, score);

                // Vai alla classifica
                gameState = GameState::SCORES;
            }
}
        else if(gameState == GameState::PAUSED) {
            glDisable(GL_DEPTH_TEST);
            textShader.use();
            glm::mat4 projection = glm::ortho(0.0f, (float)screen.w, 0.0f, (float)screen.h);
            textShader.setMat4("projection", projection);
            std::string pauseText = "GAME PAUSED";
            textRenderer.DrawText(textShader, pauseText, screen.w / 2 - 150.0f,
                screen.h / 2, 1.5f, glm::vec3(1.0f, 0.5f, 0.0f));
            std::string resumeText = "Press P to RESUME";
            textRenderer.DrawText(textShader, resumeText, screen.w / 2 - 200.0f,
                screen.h / 2 - 100.0f, 1.0f, glm::vec3(1.0f));
            std::string exitText = "Press ESC to EXIT to MENU";
            textRenderer.DrawText(textShader, exitText, screen.w / 2 - 220.0f,
                screen.h / 2 - 150.0f, 1.0f, glm::vec3(1.0f));
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
void processInput(GLFWwindow* window, FocusBox& focusBox)
{

    float currentSpeed = focusSpeed * deltaTime;
    /*
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS && glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL)
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	*/

    //Space key 
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {

        // se premi SPACE aumenta la velocità (es. raddoppia)
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            currentSpeed *= 8.0f; // boost mentre tieni premuto spazio
        }
    }
    if (gameState == GameState::PLAYING) {
        float s = focusSpeed * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) s *= 8.0f;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) focusBox.Move({ 0.0f,  s });
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) focusBox.Move({ 0.0f, -s });
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) focusBox.Move({ -s,   0.0f });
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) focusBox.Move({ s,   0.0f });

        // (opzionale) frecce:
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) focusBox.Move({ 0.0f,  s });
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) focusBox.Move({ 0.0f,-s });
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) focusBox.Move({ -s, 0.0f });
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) focusBox.Move({ s, 0.0f });
    }
    //Camera movement
    /*
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.Position += glm::vec3(0.0f, 1.0f, 0.0f) * deltaTime * 5.0f;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.Position -= glm::vec3(0.0f, 1.0f, 0.0f) * deltaTime * 5.0f;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.Position -= glm::normalize(glm::cross(camera.Front, camera.Up)) * deltaTime * 5.0f;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.Position += glm::normalize(glm::cross(camera.Front, camera.Up)) * deltaTime * 5.0f;
   
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    */
  
    //fullscreen
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !keys.keyLock[GLFW_KEY_F] && fullscreen==false)
    { 
		fullscreen = !fullscreen;
		keys.keyLock[GLFW_KEY_F] = true;
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, GLFW_DONT_CARE);

        int fbw, fbh;
        glfwGetFramebufferSize(window, &fbw, &fbh);
        framebuffer_size_callback(window, fbw, fbh);
        glm::vec2 c = focusBox.GetCenter();
        glm::vec2 half = focusBox.GetSize();
        const float padpx = 1.0f;
        c.x = glm::clamp(c.x, half.x + padpx, (float)screen.w - half.x - padpx);
        c.y = glm::clamp(c.y, half.y + padpx, (float)screen.h - half.y - padpx);
        focusBox.SetCenter(c);
        

    }
    else if(glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !keys.keyLock[GLFW_KEY_F] && fullscreen == true){
        keys.keyLock[GLFW_KEY_F] = true;
        fullscreen = false;

        // Torna a windowed e richiama il callback
        screen.resetSize(); // tua funzione
        glfwSetWindowMonitor(window, NULL, 100, 100, screen.w, screen.h, 0);
        int fbw, fbh;
        glfwGetFramebufferSize(window, &fbw, &fbh);
        framebuffer_size_callback(window, fbw, fbh);
        glm::vec2 c = focusBox.GetCenter();
        glm::vec2 half = focusBox.GetSize();
        const float padpx = 1.0f;
        c.x = glm::clamp(c.x, half.x + padpx, (float)screen.w - half.x - padpx);
        c.y = glm::clamp(c.y, half.y + padpx, (float)screen.h - half.y - padpx);
        focusBox.SetCenter(c);
    }

	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE) {
		keys.keyLock[GLFW_KEY_F] = false;
	}


	//Pause key
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !keys.keyLock[GLFW_KEY_P]) {
		keys.keyLock[GLFW_KEY_P] = true;

        if (gameState == GameState::PLAYING) {
            gameState = GameState::PAUSED;
        }
        else if (gameState == GameState::PAUSED) {
            gameState = GameState::PLAYING;

        }
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE) {
        keys.keyLock[GLFW_KEY_P] = false;
    }



	// Escape key to return to menu or exit
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS && !keys.keyLock[GLFW_KEY_ESCAPE]) {
        keys.keyLock[GLFW_KEY_ESCAPE] = true;
     
        if (gameState == GameState::PLAYING) {
                endGame = true;
                
              /*  std::cout << "\n--- FINE PARTITA ---\n";
                std::cout << "Inserisci il tuo nome: ";

			  std::string inputLine;

			  std::getline(std::cin >> std::ws, inputLine);

			  playerName = inputLine;

			  if (playerName.empty()) playerName = "Anonimo";

			  std::cerr << "[DEBUG] Nome inserito: " << playerName << "\n";

                ScoreManager::SaveScore(playerName, score);
                std::cout << "Punteggio salvato!\n";*/
                gameState = GameState::NAME_INPUT;
            
        }
        else if (gameState == GameState::SCORES || gameState == GameState::INFO || gameState==GameState::PAUSED) {
           
            gameState = GameState::MENU;            
        }
        else if(gameState == GameState::MENU ) {
            ingredients.clear();
            //active = nullptr;
            endGame = false;
            glfwSetWindowShouldClose(window, true);
		}
	}
 

    // Rilascio ESC: sblocca il keyLock
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_RELEASE) {
        keys.keyLock[GLFW_KEY_ESCAPE] = false;
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
		int viewport_height = (int)((double)width * screen.ARH / screen.ARW);
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

	float orthoX = xpos - screen.paddingW;
	float orthoY = screen.h - (ypos - screen.paddingH);
	mousePos = glm::vec2(orthoX, orthoY);

	mouseScreenPos = glm::vec2(xpos, ypos);
	//std::cout << ptr->hit(glm::vec2(xpos, ypos), *p, *v) << std::endl;

	//printf("%f | %f\n", xval, yval);


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

	if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
		camera.ProcessMouseMovement(xoffset, yoffset);


}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
}
