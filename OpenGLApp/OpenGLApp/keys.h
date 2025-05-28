#pragma once
#include <GLFW/glfw3.h>
#include <vector>


class Key {
	friend class Keys;
public:
	Key(int key): key(key) {}
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
		//Add all supported keys here
		keys.push_back(Key(GLFW_MOUSE_BUTTON_LEFT)); 
		keys.push_back(Key(GLFW_KEY_Q));
	}

	void Update(GLFWwindow* window) {
		for (Key &k : keys) {
			(k.status == GLFW_PRESS) ? (k.was_pressed = true) : (k.was_pressed = false);
			if (k.key == GLFW_MOUSE_BUTTON_LEFT || k.key == GLFW_MOUSE_BUTTON_RIGHT) {
				k.status = glfwGetMouseButton(window, k.key);
			}
			else {
				k.status = glfwGetKey(window, k.key);
			}
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
	


private:
	std::vector<Key> keys;

};