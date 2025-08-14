#pragma once
#include <GLFW/glfw3.h>
#include <vector>


class Key {
    friend class Keys;
public:
    Key(int key) : key(key) {}
private:
    int key;
    int status = -1;
    bool was_pressed = false;

    bool pressed() const {
        return status == GLFW_PRESS;
    }

    bool released() const {
        return status == GLFW_RELEASE;
    }

    bool pressed_released() const {
        return was_pressed && status == GLFW_RELEASE;
    }
};

class Keys {
public:
    Keys() {
        // Aggiungi qui i tasti che vuoi monitorare
        keys.push_back(Key(GLFW_MOUSE_BUTTON_LEFT));
        keys.push_back(Key(GLFW_KEY_Q));
        keys.push_back(Key(GLFW_KEY_P));
        // aggiungi altri se vuoi
    }

    void Update(GLFWwindow* window) {
        for (Key& k : keys) {
            int currentStatus;

            if (k.key >= GLFW_MOUSE_BUTTON_1 && k.key <= GLFW_MOUSE_BUTTON_LAST) {
                currentStatus = glfwGetMouseButton(window, k.key);
            }
            else {
                currentStatus = glfwGetKey(window, k.key);
            }

            // Update keyLock
            if (currentStatus == GLFW_RELEASE)
                keyLock[k.key] = false;

            // Salva stato precedente
            k.was_pressed = (k.status == GLFW_PRESS);

            // Aggiorna stato corrente
            k.status = currentStatus;
        }
    }

    bool Pressed(int key) const {
        for (const Key& k : keys) {
            if (k.key == key)
                return k.pressed();
        }
        return false;
    }

    bool Released(int key) const {
        for (const Key& k : keys) {
            if (k.key == key)
                return k.released();
        }
        return false;
    }

    bool PressedAndReleased(int key) const {
        for (const Key& k : keys) {
            if (k.key == key)
                return k.pressed_released();
        }
        return false;
    }

    // Blocchi accessibili esternamente
    bool keyLock[1024] = { false };

private:
    std::vector<Key> keys;
};
