#ifndef TYPES_H
#define TYPES_H
#include <cstdint>

template <typename ScalarType = float>
struct Vector2
{
    ScalarType x, y;
    Vector2()
    {
    }
    Vector2(ScalarType x, ScalarType y) : x(x), y(y)
    {
    }

    ScalarType& operator[](int idx)
    {
        return (&x)[idx];
    }
};

typedef Vector2<float> FloatVector2;

constexpr float kPi = 3.14159265358979323846f;

#endif