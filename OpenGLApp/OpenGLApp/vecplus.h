#pragma once

#include <iostream>
#include <glm/glm.hpp>

std::ostream& operator<<(std::ostream& os, const glm::vec2& v) {
    os << "(" << v.x << ", " << v.y << ")";
    return os;
}

std::ostream& operator<<(std::ostream& os, const glm::vec3& v) {
    os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    return os;
}

std::ostream& operator<<(std::ostream& os, const glm::vec4& v) {
    os << "(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")";
    return os;
}

glm::vec2 RotateVec2(glm::vec2 v, float radians) {
    float c = cos(radians);
    float s = sin(radians);
    return glm::vec2(
        c * v.x - s * v.y,
        s * v.x + c * v.y
    );
}