#pragma once

#define FOV glm::radians(45.0f)
#define CAMERA_POS glm::vec3(0.0f, 0.0f, 16.0f)

class Screen {
public:
	unsigned int w, h, paddingW, paddingH, ARW, ARH;
	glm::vec2 screenlimit;

	static Screen S4_3() {
		return Screen(320, 240, 4, 3);
	}

	static Screen S16_9() {
		return Screen(1280, 720, 16, 9);
	}

	void resetSize() {
		h = startH;
		w = startW;
	}

private:
	unsigned int startW, startH;

	Screen(unsigned int widht, unsigned int height, unsigned int ARW, unsigned int ARH)
		: w(widht), h(height), paddingW(0), paddingH(0), ARW(ARW), ARH(ARH) {
		startW = w;
		startH = h;
		float outy = tan(FOV / 2.0f) * CAMERA_POS.z; //work only for object with z = 0
		float outx = outy * ARW / ARH;
		screenlimit = glm::vec2(outx, outy);
	}

	
};

Screen screen = Screen::S16_9();