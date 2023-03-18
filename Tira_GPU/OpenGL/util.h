#pragma once

namespace GL {

constexpr float PI = 3.1415927f;

constexpr float DegreeToRadian(float degree) {
    return degree * PI / 180.0f;
}

constexpr float RadianToDegree(float radian) {
    return radian / PI * 180.0f;
}

}
