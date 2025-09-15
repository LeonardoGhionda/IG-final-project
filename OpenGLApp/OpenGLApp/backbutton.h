#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <iostream>
#include "shader.h"

extern GameState gameState;

class BackButton {
public:
    unsigned int VAO, VBO, EBO;
    unsigned int textureID;
    glm::vec2 size; // dimensione in pixel
    glm::vec2 pos;  // posizione (angolo alto sx)

    BackButton(const std::string& texturePath, glm::vec2 buttonSize, glm::vec2 screenSize)
        : size(buttonSize)
    {
        pos = glm::vec2(0.0f, screenSize.y - buttonSize.y); // angolo alto sx

        textureID = LoadTexture(texturePath);

        float vertices[] = {
            // posizioni         // texCoords
            0.0f,        0.0f,        0.0f, 0.0f,
            size.x,      0.0f,        1.0f, 0.0f,
            size.x,      size.y,      1.0f, 1.0f,
            0.0f,        size.y,      0.0f, 1.0f
        };

        unsigned int indices[] = { 0, 1, 2, 0, 2, 3 };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    void Draw(Shader& shader, glm::vec2 screenSize)
    {
        shader.use();

        glm::mat4 projection = glm::ortho(0.0f, screenSize.x, 0.0f, screenSize.y);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(pos, 0.0f));

        shader.setMat4("projection", projection);
        shader.setMat4("model", model);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        shader.setInt("texture1", 0);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    void ProcessInput(GLFWwindow* window, glm::vec2 screenSize, GameState state)
    {
        static bool wasPressed = false;

        // posizione mouse
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);
        glm::vec2 mousePos(mx, my);
        glm::vec2 correctedMouse(mousePos.x, screenSize.y - mousePos.y);

        // stato tasto
        bool isPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

        // trigger on release dentro al bottone
        if (wasPressed && !isPressed) {
            if (correctedMouse.x >= pos.x && correctedMouse.x <= pos.x + size.x &&
                correctedMouse.y >= pos.y && correctedMouse.y <= pos.y + size.y)
            {
                gameState = state;
            }
        }

        wasPressed = isPressed;
    }

private:
    unsigned int LoadTexture(const std::string& path)
    {
        unsigned int texID;
        glGenTextures(1, &texID);

        int width, height, nrChannels;
        stbi_set_flip_vertically_on_load(true);
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);

        if (data)
        {
            GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;

            glBindTexture(GL_TEXTURE_2D, texID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
        }
        else
        {
            std::cout << "Failed to load texture: " << path << std::endl;
            stbi_image_free(data);
        }

        return texID;
    }
};
