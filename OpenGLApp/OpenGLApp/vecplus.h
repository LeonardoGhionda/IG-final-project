#pragma once

#include <iostream>
#include <glm/glm.hpp>
#include <cmath> // per std::cosf, std::sinf

inline std::ostream& operator<<(std::ostream& os, const glm::vec2& v) {
    os << "(" << v.x << ", " << v.y << ")";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const glm::vec3& v) {
    os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const glm::vec4& v) {
    os << "(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")";
    return os;
}

inline glm::vec2 RotateVec2(glm::vec2 v, float radians) {
    float c = std::cosf(radians);
    float s = std::sinf(radians);
    return { c * v.x - s * v.y, s * v.x + c * v.y };
}